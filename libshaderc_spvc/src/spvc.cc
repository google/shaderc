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

#include "spvc_private.h"

#if (defined(_MSC_VER) && !defined(_CPPUNWIND)) || !defined(__EXCEPTIONS)
#define TRY_IF_EXCEPTIONS_ENABLED
#define CATCH_IF_EXCEPTIONS_ENABLED(X) if (0)
#else
#define TRY_IF_EXCEPTIONS_ENABLED try
#define CATCH_IF_EXCEPTIONS_ENABLED(X) catch (X)
#endif

struct spvc_compile_options {
  //spvc_target_env target_env = spvc_target_env_default;
  uint32_t target_env_version = 0;
};

spvc_compile_options_t spvc_compile_options_initialize() {
  return new (std::nothrow) spvc_compile_options;
}

spvc_compile_options_t spvc_compile_options_clone(
    const spvc_compile_options_t options) {
  if (!options) {
    return spvc_compile_options_initialize();
  }
  return new (std::nothrow) spvc_compile_options(*options);
}

void spvc_compile_options_release(spvc_compile_options_t options) {
  delete options;
}

spvc_compiler_t spvc_compiler_initialize() {
  spvc_compiler_t compiler = new (std::nothrow) spvc_compiler;
  return compiler;
}

void spvc_compiler_release(spvc_compiler_t compiler) { delete compiler; }

spvc_compilation_result_t spvc_compile_into_glsl(
    const spvc_compiler_t compiler,
    const uint32_t *source, size_t source_len,
    const spvc_compile_options_t additional_options) {
  auto* result = new (std::nothrow) spvc_compilation_result;
  if (!result) return nullptr;
  TRY_IF_EXCEPTIONS_ENABLED {
    spirv_cross::CompilerGLSL cross(source, source_len);
    result->output = cross.compile();
    result->compilation_status = spvc_compilation_status_success;
  }
  CATCH_IF_EXCEPTIONS_ENABLED(...) {
    result->compilation_status = spvc_compilation_status_internal_error;
  }
  return result;
}

size_t spvc_result_get_length(const spvc_compilation_result_t result) {
  return result->output_data_size;
}

size_t spvc_result_get_num_warnings(
    const spvc_compilation_result_t result) {
  return result->num_warnings;
}

size_t spvc_result_get_num_errors(
    const spvc_compilation_result_t result) {
  return result->num_errors;
}

const char* spvc_result_get_output(
    const spvc_compilation_result_t result) {
  return result->output.c_str();
}

void spvc_result_release(spvc_compilation_result_t result) {
  delete result;
}

const char* spvc_result_get_error_message(
    const spvc_compilation_result_t result) {
  return result->messages.c_str();
}

spvc_compilation_status spvc_result_get_compilation_status(
    const spvc_compilation_result_t result) {
  return result->compilation_status;
}

void spvc_get_spv_version(unsigned int* version, unsigned int* revision) {
  *version = spv::Version;
  *revision = spv::Revision;
}
