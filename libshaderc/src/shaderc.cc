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
  }
  assert(0 && "Unhandled shaderc_shader_kind");
  return EShLangVertex;
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

}  // anonymous namespace

struct shaderc_compile_options {
  shaderc_util::Compiler compiler;
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

shaderc_compiler_t shaderc_compiler_initialize() {
  std::lock_guard<std::mutex> lock(glsl_state.mutex);
  ++glsl_state.compiler_initialization_count;
  bool success = true;
  if (glsl_state.compiler_initialization_count == 1) {
    TRY_IF_EXCEPTIONS_ENABLED { success = ShInitialize(); }
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
    size_t total_warnings;
    size_t total_errors;
    EShLanguage stage = GetStage(shader_kind);
    shaderc_util::string_piece source_string =
        shaderc_util::string_piece(source_text, source_text + source_text_size);
    auto stage_function = [](std::ostream* error_stream,
                             const shaderc_util::string_piece& eror_tag) {
      return EShLangCount;
    };
    if (additional_options) {
      result->compilation_succeeded = additional_options->compiler.Compile(
          source_string, stage, "shader", stage_function, ForbidInclude(),
          &output, &errors, &total_warnings, &total_errors);
    } else {
      // Compile with default options.
      result->compilation_succeeded = shaderc_util::Compiler().Compile(
          source_string, stage, "shader", stage_function, ForbidInclude(),
          &output, &errors, &total_warnings, &total_errors);
    }
    result->messages = errors.str();
    result->spirv = output.str();
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

const char* shaderc_module_get_bytes(const shaderc_spv_module_t module) {
  return module->spirv.c_str();
}

void shaderc_module_release(shaderc_spv_module_t module) { delete module; }

const char* shaderc_module_get_error_message(
    const shaderc_spv_module_t module) {
  return module->messages.c_str();
}
