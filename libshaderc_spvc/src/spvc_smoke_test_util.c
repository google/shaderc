// Copyright 2019 The Shaderc Authors. All rights reserved.
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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "shaderc/shaderc.h"
#include "shaderc/spvc.h"

typedef shaderc_spvc_compilation_result_t (*CompileInto)(
    const shaderc_spvc_compiler_t, const uint32_t*, size_t,
    shaderc_spvc_compile_options_t);

shaderc_compilation_result_t assemble_shader(const char* shader) {
  shaderc_compiler_t shaderc;
  shaderc = shaderc_compiler_initialize();
  shaderc_compile_options_t opt = shaderc_compile_options_initialize();
  shaderc_compilation_result_t res =
      shaderc_assemble_into_spv(shaderc, shader, strlen(shader), opt);
  shaderc_compile_options_release(opt);
  shaderc_compiler_release(shaderc);
  return res;
}

int test_exec(shaderc_spvc_compiler_t compiler,
              shaderc_spvc_compile_options_t options,
              shaderc_compilation_result_t assembled_shader,
              CompileInto compile_into, const char* target_lang) {
  shaderc_spvc_compilation_result_t result;
  result = compile_into(
      compiler, (const uint32_t*)shaderc_result_get_bytes(assembled_shader),
      shaderc_result_get_length(assembled_shader) / sizeof(uint32_t), options);
  assert(result);

  int ret_val;
  if (shaderc_spvc_result_get_status(result) ==
      shaderc_compilation_status_success) {
    printf("success! %lu characters of %s\n",
           (unsigned long)(strlen(shaderc_spvc_result_get_output(result))),
           target_lang);
    ret_val = 1;
  } else {
    printf("failed to produce %s\n", target_lang);
    ret_val = 0;
  }

  shaderc_spvc_result_release(result);
  return ret_val;
}

int run_glsl(shaderc_spvc_compiler_t compiler,
             shaderc_spvc_compile_options_t options,
             shaderc_compilation_result_t assembled_shader) {
  return test_exec(compiler, options, assembled_shader,
                   shaderc_spvc_compile_into_glsl, "glsl");
}

int run_hlsl(shaderc_spvc_compiler_t compiler,
             shaderc_spvc_compile_options_t options,
             shaderc_compilation_result_t assembled_shader) {
  return test_exec(compiler, options, assembled_shader,
                   shaderc_spvc_compile_into_hlsl, "hlsl");
}

int run_msl(shaderc_spvc_compiler_t compiler,
            shaderc_spvc_compile_options_t options,
            shaderc_compilation_result_t assembled_shader) {
  return test_exec(compiler, options, assembled_shader,
                   shaderc_spvc_compile_into_msl, "msl");
}

int run_smoke_test(const char* shader, int transform_from_webgpu) {
  shaderc_compilation_result_t assembled_shader = assemble_shader(shader);
  if (assembled_shader == NULL) {
    printf("failed to assemble input!\n");
    return -1;
  }

  int ret_code = 0;
  shaderc_spvc_compiler_t compiler = shaderc_spvc_compiler_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();
  if (transform_from_webgpu) {
    shaderc_spvc_compile_options_set_source_env(
        options, shaderc_target_env_webgpu, shaderc_env_version_webgpu);
    shaderc_spvc_compile_options_set_webgpu_to_vulkan(options,
                                                      transform_from_webgpu);
  }

  if (!run_glsl(compiler, options, assembled_shader)) ret_code = -1;
  if (!run_hlsl(compiler, options, assembled_shader)) ret_code = -1;
  if (!run_msl(compiler, options, assembled_shader)) ret_code = -1;

  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_compiler_release(compiler);
  shaderc_result_release(assembled_shader);

  return ret_code;
}
