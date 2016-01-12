// Copyright 2015 The Shaderc Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "shaderc_private.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <vector>

#include "SPIRV/spirv.hpp"

#include "libshaderc_util/compiler.h"
#include "libshaderc_util/resources.h"

#if (defined(_MSC_VER) && !defined(_CPPUNWIND)) || !defined(__EXCEPTIONS)
#define TRY_IF_EXCEPTIONS_ENABLED
#define CATCH_IF_EXCEPTIONS_ENABLED(X) if (0)
#else
#define TRY_IF_EXCEPTIONS_ENABLED try
#define CATCH_IF_EXCEPTIONS_ENABLED(X) catch (X)
#endif

namespace {

// Returns shader stage (ie: vertex, fragment, etc.) corresponding to kind.
EShLanguage GetStage(shaderc_shader_kind kind) {
  switch (kind) {
    case shaderc_glsl_vertex_shader:
      return EShLangVertex;
    case shaderc_glsl_fragment_shader:
      return EShLangFragment;
    case shaderc_glsl_compute_shader:
      return EShLangCompute;
    case shaderc_glsl_geometry_shader:
      return EShLangGeometry;
    case shaderc_glsl_tess_control_shader:
      return EShLangTessControl;
    case shaderc_glsl_tess_evaluation_shader:
      return EShLangTessEvaluation;
  }
  assert(0 && "Unhandled shaderc_shader_kind");
  return EShLangVertex;
}

// Converts shaderc_target_env to EShMessages
EShMessages GetMessageRules(shaderc_target_env target) {
  EShMessages msgs = EShMsgDefault;

  switch(target) {
    case shaderc_target_env_opengl_compat:
      break;
    case shaderc_target_env_opengl:
      msgs = EShMsgSpvRules;
      break;
    case shaderc_target_env_vulkan:
      msgs = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
      break;
  }

  return msgs;
}

struct {
  // The number of currently initialized compilers.
  int compiler_initialization_count;

  std::mutex mutex;  // Guards creation and deletion of glsl state.
} glsl_state = {
    0,
};

std::mutex compile_mutex;  // Guards shaderc_compile_*.

// Rejects #include directives.
class ForbidInclude : public shaderc_util::CountingIncluder {
 private:
  // Returns empty contents and an error message.  See base-class version.
  std::pair<std::string, std::string> include_delegate(
      const char* filename) const override {
    return std::make_pair<std::string, std::string>(
        "", "unexpected include directive");
  }
};

// QINING
class FileIncluder : public shaderc_util::CountingIncluder {
 private:
  bool AreValidCallbacks() const {
    return callbacks_.get_fullpath_and_content!= nullptr &&
           callbacks_.finalize_including!= nullptr;
  }

  // Find filename in search path and returns its contents.
  std::pair<std::string, std::string> include_delegate(
      const char* filename) const override {
    if (!AreValidCallbacks())
      return std::make_pair<std::string, std::string>(
          "", "unexpected include directive");
    // size_t full_path_length = callbacks_.fullpath_size_getter(filename);
    ////char* full_path_ptr = new char[full_path_length];
    // std::unique_ptr<char[]> full_path(new char[full_path_length]);
    // char* full_path_ptr = full_path.get();
    // callbacks_.fullpath_data_getter(filename, &full_path_ptr);
    // size_t content_length = callbacks_.content_size_getter(filename);
    ////char* content_ptr = new char[content_length];
    // std::unique_ptr<char[]> content(new char[content_length]);
    // char* content_ptr = content.get();
    // callbacks_.content_data_getter(filename, &content_ptr);

    // return std::pair<std::string, std::string>(
    //    std::string(full_path_ptr, full_path_length),
    //    std::string(content_ptr, content_length));

    shaderc_includer_fullpath_content data =
        callbacks_.get_fullpath_and_content(callbacks_.getter_user_data,
                                            filename);
    std::pair<std::string, std::string> entry =
        std::make_pair<std::string, std::string>(
            std::string(data.fullpath, data.fullpath_length),
            std::string(data.content, data.content_length));
    callbacks_.finalize_including(callbacks_.finalizer_user_data, filename,
                                  data);
    return entry;
  }

  shaderc_includer_callbacks callbacks_;

 public:
  FileIncluder(const shaderc_includer_callbacks& callbacks)
      : callbacks_(callbacks){};
  FileIncluder() : callbacks_({nullptr, nullptr, nullptr, nullptr}){};
};

}  // anonymous namespace

struct shaderc_compile_options {
  shaderc_compile_options(){};
  shaderc_util::Compiler compiler;
  shaderc_includer_callbacks includer_callbacks = {nullptr, nullptr, nullptr, nullptr};
};

shaderc_compile_options_t shaderc_compile_options_initialize() {
  return new (std::nothrow) shaderc_compile_options;
}

shaderc_compile_options_t shaderc_compile_options_clone(
    const shaderc_compile_options_t options) {
  if (!options) {
    return shaderc_compile_options_initialize();
  }
  return new (std::nothrow) shaderc_compile_options(*options);
}

void shaderc_compile_options_release(shaderc_compile_options_t options) {
  delete options;
}

void shaderc_compile_options_add_macro_definition(
    shaderc_compile_options_t options, const char* name, const char* value) {
  options->compiler.AddMacroDefinition(name, value);
}

void shaderc_compile_options_set_generate_debug_info(
    shaderc_compile_options_t options) {
  options->compiler.SetGenerateDebugInfo();
}

void shaderc_compile_options_set_disassembly_mode(
    shaderc_compile_options_t options) {
  options->compiler.SetDisassemblyMode();
}

void shaderc_compile_options_set_forced_version_profile(
    shaderc_compile_options_t options, int version, shaderc_profile profile) {
  // Transfer the profile parameter from public enum type to glslang internal
  // enum type. No default case here so that compiler will complain if new enum
  // member is added later but not handled here.
  switch(profile){
    case shaderc_profile_none:
      options->compiler.SetForcedVersionProfile(version, ENoProfile);
      break;
    case shaderc_profile_core:
      options->compiler.SetForcedVersionProfile(version, ECoreProfile);
      break;
    case shaderc_profile_compatibility:
      options->compiler.SetForcedVersionProfile(version, ECompatibilityProfile);
      break;
    case shaderc_profile_es:
      options->compiler.SetForcedVersionProfile(version, EEsProfile);
      break;
  }
}

void shaderc_compile_options_set_includer_callbacks(
    shaderc_compile_options_t options,
    const shaderc_includer_callbacks* callbacks_ptr) {
  options->includer_callbacks = *callbacks_ptr;
}

void shaderc_compile_options_set_preprocessing_only_mode(
    shaderc_compile_options_t options) {
  options->compiler.SetPreprocessingOnlyMode();
}

void shaderc_compile_options_set_suppress_warnings(
    shaderc_compile_options_t options) {
  options->compiler.SetSuppressWarnings();
}

void shaderc_compile_options_set_target_env(
    shaderc_compile_options_t options, shaderc_target_env target, uint32_t version) {
  // "version" reserved for future use, intended to distinguish between different
  // versions of a target environment
  options->compiler.SetMessageRules(GetMessageRules(target));
}

void shaderc_compile_options_set_warnings_as_errors(
    shaderc_compile_options_t options) {
  options->compiler.SetWarningsAsErrors();
}

shaderc_compiler_t shaderc_compiler_initialize() {
  std::lock_guard<std::mutex> lock(glsl_state.mutex);
  ++glsl_state.compiler_initialization_count;
  bool success = true;
  if (glsl_state.compiler_initialization_count == 1) {
    TRY_IF_EXCEPTIONS_ENABLED { success = (ShInitialize() != 0); }
    CATCH_IF_EXCEPTIONS_ENABLED(...) { success = false; }
  }
  if (!success) {
    glsl_state.compiler_initialization_count -= 1;
    return nullptr;
  }
  shaderc_compiler_t compiler = new (std::nothrow) shaderc_compiler;
  if (!compiler) {
    return nullptr;
  }

  return compiler;
}

void shaderc_compiler_release(shaderc_compiler_t compiler) {
  if (compiler == nullptr) {
    return;
  }

  if (compiler->initialized) {
    compiler->initialized = false;
    delete compiler;
    // Defend against further dereferences through the "compiler" variable.
    compiler = nullptr;
    std::lock_guard<std::mutex> lock(glsl_state.mutex);
    if (glsl_state.compiler_initialization_count) {
      --glsl_state.compiler_initialization_count;
      if (glsl_state.compiler_initialization_count == 0) {
        TRY_IF_EXCEPTIONS_ENABLED { ShFinalize(); }
        CATCH_IF_EXCEPTIONS_ENABLED(...) {}
      }
    }
  }
}

shaderc_spv_module_t shaderc_compile_into_spv(
    const shaderc_compiler_t compiler, const char* source_text,
    size_t source_text_size, shaderc_shader_kind shader_kind,
    const char* entry_point_name,
    const shaderc_compile_options_t additional_options) {
  std::lock_guard<std::mutex> lock(compile_mutex);

  shaderc_spv_module_t result = new (std::nothrow) shaderc_spv_module;
  if (!result) {
    return nullptr;
  }

  result->compilation_succeeded = false;  // In case we exit early.
  if (!compiler->initialized) return result;
  TRY_IF_EXCEPTIONS_ENABLED {
    std::stringstream output;
    std::stringstream errors;
    size_t total_warnings = 0;
    size_t total_errors = 0;
    EShLanguage stage = GetStage(shader_kind);
    shaderc_util::string_piece source_string =
        shaderc_util::string_piece(source_text, source_text + source_text_size);
    auto stage_function = [](std::ostream* error_stream,
                             const shaderc_util::string_piece& eror_tag) {
      return EShLangCount;
    };
    if (additional_options) {
      result->compilation_succeeded = additional_options->compiler.Compile(
          source_string, stage, "shader", stage_function,
          FileIncluder(additional_options->includer_callbacks), &output,
          &errors, &total_warnings, &total_errors);
    } else {
      // Compile with default options.
      result->compilation_succeeded = shaderc_util::Compiler().Compile(
          source_string, stage, "shader", stage_function,
          FileIncluder(), &output,
          &errors, &total_warnings, &total_errors);
    }
    result->messages = errors.str();
    result->spirv = output.str();
    result->num_warnings = total_warnings;
    result->num_errors = total_errors;
  }
  CATCH_IF_EXCEPTIONS_ENABLED(...) { result->compilation_succeeded = false; }
  return result;
}

bool shaderc_module_get_success(const shaderc_spv_module_t module) {
  return module->compilation_succeeded;
}

size_t shaderc_module_get_length(const shaderc_spv_module_t module) {
  return module->spirv.size();
}

size_t shaderc_module_get_num_warnings(const shaderc_spv_module_t module) {
  return module->num_warnings;
}

size_t shaderc_module_get_num_errors(const shaderc_spv_module_t module) {
  return module->num_errors;
}

const char* shaderc_module_get_bytes(const shaderc_spv_module_t module) {
  return module->spirv.c_str();
}

void shaderc_module_release(shaderc_spv_module_t module) { delete module; }

const char* shaderc_module_get_error_message(
    const shaderc_spv_module_t module) {
  return module->messages.c_str();
}

void shaderc_get_spv_version(unsigned int *version, unsigned int *revision) {
    *version = spv::Version;
    *revision = spv::Revision;
}
