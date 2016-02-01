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
#include <sstream>
#include <vector>

#include "SPIRV/spirv.hpp"

#include "libshaderc_util/compiler.h"
#include "libshaderc_util/resources.h"
#include "libshaderc_util/version_profile.h"

#if (defined(_MSC_VER) && !defined(_CPPUNWIND)) || !defined(__EXCEPTIONS)
#define TRY_IF_EXCEPTIONS_ENABLED
#define CATCH_IF_EXCEPTIONS_ENABLED(X) if (0)
#else
#define TRY_IF_EXCEPTIONS_ENABLED try
#define CATCH_IF_EXCEPTIONS_ENABLED(X) catch (X)
#endif

namespace {

// Returns shader stage (ie: vertex, fragment, etc.) in response to forced
// shader kinds. If the shader kind is not a forced kind, returns EshLangCount
// to let #pragma annotation or shader stage deducer determine the stage to
// use.
EShLanguage GetForcedStage(shaderc_shader_kind kind) {
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
    case shaderc_glsl_infer_from_source:
    case shaderc_glsl_default_vertex_shader:
    case shaderc_glsl_default_fragment_shader:
    case shaderc_glsl_default_compute_shader:
    case shaderc_glsl_default_geometry_shader:
    case shaderc_glsl_default_tess_control_shader:
    case shaderc_glsl_default_tess_evaluation_shader:
      return EShLangCount;
  }
  assert(0 && "Unhandled shaderc_shader_kind");
  return EShLangCount;
}

// Converts shaderc_target_env to EShMessages
EShMessages GetMessageRules(shaderc_target_env target) {
  EShMessages msgs = EShMsgDefault;

  switch (target) {
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

// A wrapper functor class to be used as stage deducer for libshaderc_util
// Compile() interface. When the given shader kind is one of the default shader
// kinds, this functor will be called if #pragma is not found in the source
// code. And it returns the corresponding shader stage. When the shader kind is
// a forced shader kind, this functor won't be called and it simply returns
// EShLangCount to make the syntax correct. When the shader kind is set to
// shaderc_glsl_deduce_from_pragma, this functor also returns EShLangCount, but
// the compiler should emit error if #pragma annotation is not found in this
// case.
class StageDeducer {
 public:
  StageDeducer(shaderc_shader_kind kind = shaderc_glsl_infer_from_source)
      : kind_(kind), error_(false){};
  // The method that underlying glslang will call to determine the shader stage
  // to be used in current compilation. It is called only when there is neither
  // forced shader kind (or say stage, in the view of glslang), nor #pragma
  // annotation in the source code. This method transforms an user defined
  // 'default' shader kind to the corresponding shader stage. As this is the
  // last trial to determine the shader stage, failing to find the corresponding
  // shader stage will record an error.
  // Note that calling this method more than once during one compilation will
  // have the error recorded for the previous call been overwriten by the next
  // call.
  EShLanguage operator()(std::ostream* /*error_stream*/,
                         const shaderc_util::string_piece& /*error_tag*/) {
    EShLanguage stage = GetDefaultStage(kind_);
    if (stage == EShLangCount) {
      error_ = true;
    } else {
      error_ = false;
    }
    return stage;
  };

  // Returns true if there is error during shader stage deduction.
  bool error() const { return error_; }

 private:
  // Gets the corresponding shader stage for a given 'default' shader kind. All
  // other kinds are mapped to EShLangCount which should not be used.
  EShLanguage GetDefaultStage(shaderc_shader_kind kind) const {
    switch (kind) {
      case shaderc_glsl_vertex_shader:
      case shaderc_glsl_fragment_shader:
      case shaderc_glsl_compute_shader:
      case shaderc_glsl_geometry_shader:
      case shaderc_glsl_tess_control_shader:
      case shaderc_glsl_tess_evaluation_shader:
      case shaderc_glsl_infer_from_source:
        return EShLangCount;
      case shaderc_glsl_default_vertex_shader:
        return EShLangVertex;
      case shaderc_glsl_default_fragment_shader:
        return EShLangFragment;
      case shaderc_glsl_default_compute_shader:
        return EShLangCompute;
      case shaderc_glsl_default_geometry_shader:
        return EShLangGeometry;
      case shaderc_glsl_default_tess_control_shader:
        return EShLangTessControl;
      case shaderc_glsl_default_tess_evaluation_shader:
        return EShLangTessEvaluation;
    }
    assert(0 && "Unhandled shaderc_shader_kind");
    return EShLangCount;
  }

  shaderc_shader_kind kind_;
  bool error_;
};

// A bridge between the libshaderc includer and libshaderc_util includer.
class InternalFileIncluder : public shaderc_util::CountingIncluder {
 public:
  InternalFileIncluder(
      const shaderc_includer_response_get_fn get_includer_response,
      const shaderc_includer_response_release_fn release_includer_response,
      void* user_data)
      : get_includer_response_(get_includer_response),
        release_includer_response_(release_includer_response),
        user_data_(user_data){};
  InternalFileIncluder()
      : get_includer_response_(nullptr),
        release_includer_response_(nullptr),
        user_data_(nullptr){};

 private:
  // Check the validity of the callbacks.
  bool AreValidCallbacks() const {
    return get_includer_response_ != nullptr &&
           release_includer_response_ != nullptr;
  }

  // Find filename in search path and returns its contents.
  std::pair<std::string, std::string> include_delegate(
      const char* filename) const override {
    if (!AreValidCallbacks())
      return std::make_pair<std::string, std::string>(
          "", "unexpected include directive");
    shaderc_includer_response* data =
        get_includer_response_(user_data_, filename);
    std::pair<std::string, std::string> entry =
        std::make_pair(std::string(data->path, data->path_length),
                       std::string(data->content, data->content_length));
    release_includer_response_(user_data_, data);
    return entry;
  }

  const shaderc_includer_response_get_fn get_includer_response_;
  const shaderc_includer_response_release_fn release_includer_response_;
  void* user_data_;
};

}  // anonymous namespace

struct shaderc_compile_options {
  shaderc_compile_options(){};
  shaderc_util::Compiler compiler;
  shaderc_includer_response_get_fn get_includer_response;
  shaderc_includer_response_release_fn release_includer_response;
  void* includer_user_data;
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
    shaderc_compile_options_t options, const char* name, size_t name_length,
    const char* value, size_t value_length) {
  options->compiler.AddMacroDefinition(name, name_length, value, value_length);
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
  switch (profile) {
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
    shaderc_includer_response_get_fn get_includer_response,
    shaderc_includer_response_release_fn release_includer_response,
    void* user_data) {
  options->get_includer_response = get_includer_response;
  options->release_includer_response = release_includer_response;
  options->includer_user_data = user_data;
}

void shaderc_compile_options_set_preprocessing_only_mode(
    shaderc_compile_options_t options) {
  options->compiler.SetPreprocessingOnlyMode();
}

void shaderc_compile_options_set_suppress_warnings(
    shaderc_compile_options_t options) {
  options->compiler.SetSuppressWarnings();
}

void shaderc_compile_options_set_target_env(shaderc_compile_options_t options,
                                            shaderc_target_env target,
                                            uint32_t version) {
  // "version" reserved for future use, intended to distinguish between
  // different versions of a target environment
  options->compiler.SetMessageRules(GetMessageRules(target));
}

void shaderc_compile_options_set_warnings_as_errors(
    shaderc_compile_options_t options) {
  options->compiler.SetWarningsAsErrors();
}

shaderc_compiler_t shaderc_compiler_initialize() {
  static shaderc_util::GlslInitializer* initializer =
      new shaderc_util::GlslInitializer;
  shaderc_compiler_t compiler = new (std::nothrow) shaderc_compiler;
  compiler->initializer = initializer;
  return compiler;
}

void shaderc_compiler_release(shaderc_compiler_t compiler) { delete compiler; }

shaderc_spv_module_t shaderc_compile_into_spv(
    const shaderc_compiler_t compiler, const char* source_text,
    size_t source_text_size, shaderc_shader_kind shader_kind,
    const char* input_file_name, const char* entry_point_name,
    const shaderc_compile_options_t additional_options) {
  shaderc_spv_module_t result = new (std::nothrow) shaderc_spv_module;
  if (!result) {
    return nullptr;
  }
  result->compilation_status = shaderc_compilation_status_invalid_stage;
  bool compilation_succeeded = false;  // In case we exit early.
  if (!compiler->initializer) return result;
  TRY_IF_EXCEPTIONS_ENABLED {
    std::stringstream output;
    std::stringstream errors;
    size_t total_warnings = 0;
    size_t total_errors = 0;
    std::string input_file_name_str(input_file_name);
    EShLanguage forced_stage = GetForcedStage(shader_kind);
    shaderc_util::string_piece source_string =
        shaderc_util::string_piece(source_text, source_text + source_text_size);
    StageDeducer stage_deducer(shader_kind);
    if (additional_options) {
      compilation_succeeded = additional_options->compiler.Compile(
          source_string, forced_stage, input_file_name_str,
          // stage_deducer has a flag: error_, which we need to check later.
          // We need to make this a reference wrapper, so that std::function
          // won't make a copy for this callable object.
          std::ref(stage_deducer),
          InternalFileIncluder(additional_options->get_includer_response,
                               additional_options->release_includer_response,
                               additional_options->includer_user_data),
          &output, &errors, &total_warnings, &total_errors,
          compiler->initializer);
    } else {
      // Compile with default options.
      compilation_succeeded = shaderc_util::Compiler().Compile(
          source_string, forced_stage, input_file_name_str,
          std::ref(stage_deducer), InternalFileIncluder(), &output, &errors,
          &total_warnings, &total_errors, compiler->initializer);
    }

    result->messages = errors.str();
    result->spirv = output.str();
    result->num_warnings = total_warnings;
    result->num_errors = total_errors;
    if (compilation_succeeded) {
      result->compilation_status = shaderc_compilation_status_success;
    } else {
      // Check whether the error is caused by failing to deduce the shader
      // stage. If it is the case, set the error type to shader kind error.
      // Otherwise, set it to compilation error.
      result->compilation_status =
          stage_deducer.error() ? shaderc_compilation_status_invalid_stage
                                : shaderc_compilation_status_compilation_error;
    }
  }
  CATCH_IF_EXCEPTIONS_ENABLED(...) {
    result->compilation_status = shaderc_compilation_status_internal_error;
  }
  return result;
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

shaderc_compilation_status shaderc_module_get_compilation_status(
    const shaderc_spv_module_t module) {
  return module->compilation_status;
}

void shaderc_get_spv_version(unsigned int* version, unsigned int* revision) {
  *version = spv::Version;
  *revision = spv::Revision;
}

bool shaderc_parse_version_profile(const char* str, int* version,
                                   shaderc_profile* profile) {
  EProfile glslang_profile;
  bool success = shaderc_util::ParseVersionProfile(
      std::string(str, strlen(str)), version, &glslang_profile);
  if (!success) return false;

  switch (glslang_profile) {
    case EEsProfile:
      *profile = shaderc_profile_es;
      return true;
    case ECoreProfile:
      *profile = shaderc_profile_core;
      return true;
    case ECompatibilityProfile:
      *profile = shaderc_profile_compatibility;
      return true;
    case ENoProfile:
      *profile = shaderc_profile_none;
      return true;
    case EBadProfile:
      return false;
  }

  // Shouldn't reach here, all profile enum should be handled above.
  // Be strict to return false.
  return false;
}
