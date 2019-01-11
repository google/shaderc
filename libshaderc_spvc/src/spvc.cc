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

#include "spirv-tools/libspirv.hpp"
#include "spirv-cross/spirv_glsl.hpp"
#include "spirv-cross/spirv_hlsl.hpp"
#include "spirv-cross/spirv_msl.hpp"

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
}

shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_glsl(
    const shaderc_spvc_compiler_t compiler, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {

  auto* result = new (std::nothrow) shaderc_spvc_compilation_result;
  if (!result) return nullptr;

  if (options->validate) {
    spvtools::SpirvTools tools(options->target_env);
    tools.SetMessageConsumer(std::bind(
        consume_validation_message, result, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    if (!tools.Validate(source, source_len, spvtools::ValidatorOptions())) {
      result->status = shaderc_compilation_status_validation_error;
      return result;
    }
  }

  spirv_cross::CompilerGLSL cross(source, source_len);
  TRY_IF_EXCEPTIONS_ENABLED {
    cross.set_common_options(options->glsl);
    result->output = cross.compile();
    // An exception during compiling would crash (if exceptions off) or jump to
    // the catch block (if exceptions on) so if we're here we know the compile
    // worked.
    result->status = shaderc_compilation_status_success;
  }
  CATCH_IF_EXCEPTIONS_ENABLED(...) {
    result->status = shaderc_compilation_status_compilation_error;
    result->messages = "Compilation failed.  Partial source:";
    result->messages.append(cross.get_partial_source());
  }
  return result;
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
