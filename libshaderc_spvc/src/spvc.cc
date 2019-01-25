// Copyright 2018 The Shaderc Authors. All rights reserved.
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

#include "shaderc/spvc.h"
#include "libshaderc_util/exceptions.h"

#include "spirv-cross/spirv_glsl.hpp"
#include "spirv-cross/spirv_hlsl.hpp"
#include "spirv-cross/spirv_msl.hpp"
#include "spirv-tools/libspirv.hpp"

struct shaderc_spvc_compiler {};

// Described in spvc.h.
struct shaderc_spvc_compilation_result {
  std::string output;
  std::string messages;
  shaderc_compilation_status status =
      shaderc_compilation_status_null_result_object;
};

struct shaderc_spvc_compile_options {
  bool validate = true;
  spv_target_env target_env = SPV_ENV_VULKAN_1_0;
  spirv_cross::CompilerGLSL::Options glsl;
  spirv_cross::CompilerHLSL::Options hlsl;
  spirv_cross::CompilerMSL::Options msl;
};

shaderc_spvc_compile_options_t shaderc_spvc_compile_options_initialize() {
  return new (std::nothrow) shaderc_spvc_compile_options;
}

shaderc_spvc_compile_options_t shaderc_spvc_compile_options_clone(
    shaderc_spvc_compile_options_t options) {
  if (options) return new (std::nothrow) shaderc_spvc_compile_options(*options);
  return shaderc_spvc_compile_options_initialize();
}

void shaderc_spvc_compile_options_release(
    shaderc_spvc_compile_options_t options) {
  delete options;
}

void shaderc_spvc_compile_options_set_target_env(
    shaderc_spvc_compile_options_t options, shaderc_target_env target,
    shaderc_env_version version) {
  switch (target) {
    case shaderc_target_env_opengl:
    case shaderc_target_env_opengl_compat:
      switch (version) {
        case shaderc_env_version_opengl_4_5:
          options->target_env = SPV_ENV_OPENGL_4_5;
          break;
        default:
          break;
      }
      break;
    case shaderc_target_env_vulkan:
      switch (version) {
        case shaderc_env_version_vulkan_1_0:
          options->target_env = SPV_ENV_VULKAN_1_0;
          break;
        case shaderc_env_version_vulkan_1_1:
          options->target_env = SPV_ENV_VULKAN_1_1;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void shaderc_spvc_compile_options_set_output_language_version(
    shaderc_spvc_compile_options_t options, uint32_t version) {
  options->glsl.version = version;
}

void shaderc_spvc_compile_options_set_shader_model(
    shaderc_spvc_compile_options_t options, uint32_t model) {
  options->hlsl.shader_model = model;
}

void shaderc_spvc_compile_options_set_fixup_clipspace(
    shaderc_spvc_compile_options_t options, bool b) {
  options->glsl.vertex.fixup_clipspace = b;
}

void shaderc_spvc_compile_options_set_flip_vert_y(
    shaderc_spvc_compile_options_t options, bool b) {
  options->glsl.vertex.flip_vert_y = b;
}

size_t shaderc_spvc_compile_options_set_for_fuzzing(
    shaderc_spvc_compile_options_t options, const uint8_t* data, size_t size) {
  if (!data || size < sizeof(*options)) return 0;

  memcpy(options, data, sizeof(*options));
  return sizeof(*options);
}

shaderc_spvc_compiler_t shaderc_spvc_compiler_initialize() {
  return new (std::nothrow) shaderc_spvc_compiler;
}

void shaderc_spvc_compiler_release(shaderc_spvc_compiler_t compiler) {
  delete compiler;
}

namespace {
void consume_validation_message(shaderc_spvc_compilation_result* result,
                                spv_message_level_t level, const char* src,
                                const spv_position_t& pos,
                                const char* message) {
  result->messages.append(message);
  result->messages.append("\n");
}

// Validate the source spir-v if requested, and if valid use the given compiler
// to translate it to a higher level language. CompilerGLSL is the base class
// for all spirv-cross compilers so this function works with a compiler for any
// output language. The given compiler should already have its options set by
// the caller.
shaderc_spvc_compilation_result_t validate_and_compile(
    spirv_cross::CompilerGLSL* compiler, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {
  auto* result = new (std::nothrow) shaderc_spvc_compilation_result;
  if (!result) return nullptr;

  if (options->validate) {
    spvtools::SpirvTools tools(options->target_env);
    if (!tools.IsValid()) return nullptr;
    tools.SetMessageConsumer(std::bind(
        consume_validation_message, result, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    if (!tools.Validate(source, source_len, spvtools::ValidatorOptions())) {
      result->status = shaderc_compilation_status_validation_error;
      return result;
    }
  }

  TRY_IF_EXCEPTIONS_ENABLED {
    result->output = compiler->compile();
    // An exception during compiling would crash (if exceptions off) or jump to
    // the catch block (if exceptions on) so if we're here we know the compile
    // worked.
    result->status = shaderc_compilation_status_success;
  }
  CATCH_IF_EXCEPTIONS_ENABLED(...) {
    result->status = shaderc_compilation_status_compilation_error;
    result->messages = "Compilation failed.  Partial source:";
    result->messages.append(compiler->get_partial_source());
  }
  return result;
}
}  // namespace

shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_glsl(
    const shaderc_spvc_compiler_t, const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options) {
  spirv_cross::CompilerGLSL compiler(source, source_len);
  compiler.set_common_options(options->glsl);
  return validate_and_compile(&compiler, source, source_len, options);
}

shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_hlsl(
    const shaderc_spvc_compiler_t, const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options) {
  spirv_cross::CompilerHLSL compiler(source, source_len);
  compiler.set_common_options(options->glsl);
  compiler.set_hlsl_options(options->hlsl);
  return validate_and_compile(&compiler, source, source_len, options);
}

shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_msl(
    const shaderc_spvc_compiler_t, const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options) {
  spirv_cross::CompilerMSL compiler(source, source_len);
  compiler.set_common_options(options->glsl);
  return validate_and_compile(&compiler, source, source_len, options);
}

const char* shaderc_spvc_result_get_output(
    const shaderc_spvc_compilation_result_t result) {
  return result->output.c_str();
}

const char* shaderc_spvc_result_get_messages(
    const shaderc_spvc_compilation_result_t result) {
  return result->messages.c_str();
}

shaderc_compilation_status shaderc_spvc_result_get_status(
    const shaderc_spvc_compilation_result_t result) {
  return result->status;
}

void shaderc_spvc_result_release(shaderc_spvc_compilation_result_t result) {
  delete result;
}
