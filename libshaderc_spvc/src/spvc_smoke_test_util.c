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
#include "spvc/spvc.h"

typedef shaderc_spvc_status (*InitCmd)(const shaderc_spvc_context_t,
                                       const uint32_t*, size_t,
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

int test_exec(shaderc_spvc_context_t context,
              shaderc_spvc_compile_options_t options,
              shaderc_compilation_result_t assembled_shader, InitCmd init_cmd,
              const char* target_lang) {
  shaderc_spvc_compilation_result_t result = shaderc_spvc_result_create();
  assert(result);
  shaderc_spvc_status status = init_cmd(
      context, (const uint32_t*)shaderc_result_get_bytes(assembled_shader),
      shaderc_result_get_length(assembled_shader) / sizeof(uint32_t), options);
  int ret_val;
  if (status == shaderc_spvc_status_success) {
    shaderc_spvc_status compile_status =
        shaderc_spvc_compile_shader(context, result);
    if (compile_status == shaderc_spvc_status_success) {
      const char* result_str;
      shaderc_spvc_result_get_string_output(result, &result_str);
      printf("success! %lu characters of %s\n",
             (unsigned long)(strlen(result_str)),
             target_lang);
      ret_val = 1;
    } else {
      printf("failed to produce %s\n", target_lang);
      ret_val = 0;
    }
  } else {
    printf("failed to initialize %s\n", target_lang);
    ret_val = 0;
  }

  shaderc_spvc_result_destroy(result);
  return ret_val;
}

int run_glsl(shaderc_spvc_context_t context,
             shaderc_spvc_compile_options_t options,
             shaderc_compilation_result_t assembled_shader) {
  return test_exec(context, options, assembled_shader,
                   shaderc_spvc_initialize_for_glsl, "glsl");
}

int run_hlsl(shaderc_spvc_context_t context,
             shaderc_spvc_compile_options_t options,
             shaderc_compilation_result_t assembled_shader) {
  return test_exec(context, options, assembled_shader,
                   shaderc_spvc_initialize_for_hlsl, "hlsl");
}

int run_msl(shaderc_spvc_context_t context,
            shaderc_spvc_compile_options_t options,
            shaderc_compilation_result_t assembled_shader) {
  return test_exec(context, options, assembled_shader,
                   shaderc_spvc_initialize_for_msl, "msl");
}

int run_smoke_test(const char* shader, int transform_from_webgpu) {
  shaderc_compilation_result_t assembled_shader = assemble_shader(shader);
  if (assembled_shader == NULL) {
    printf("failed to assemble input!\n");
    return -1;
  }

  int ret_code = 0;
  shaderc_spvc_context_t context = shaderc_spvc_context_create();
  shaderc_spvc_compile_options_t options;
  if (transform_from_webgpu) {
    options = shaderc_spvc_compile_options_create(
        shaderc_spvc_spv_env_webgpu_0, shaderc_spvc_spv_env_vulkan_1_1);
  } else {
    options = shaderc_spvc_compile_options_create(
        shaderc_spvc_spv_env_vulkan_1_0, shaderc_spvc_spv_env_vulkan_1_0);
  }

  if (!run_glsl(context, options, assembled_shader)) ret_code = -1;
  if (!run_hlsl(context, options, assembled_shader)) ret_code = -1;
  if (!run_msl(context, options, assembled_shader)) ret_code = -1;

  shaderc_spvc_compile_options_destroy(options);
  shaderc_spvc_context_destroy(context);
  shaderc_result_release(assembled_shader);

  return ret_code;
}
