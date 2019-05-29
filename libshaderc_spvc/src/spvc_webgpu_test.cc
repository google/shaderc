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

#include <gtest/gtest.h>

#include <thread>

#include "common_shaders_for_test.h"
#include "shaderc/spvc.h"

namespace {

TEST(Compile, Glsl) {
  shaderc_spvc_compiler_t compiler = shaderc_spvc_compiler_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compile_options_set_source_env(
      options, shaderc_target_env_webgpu, shaderc_env_version_webgpu);
  shaderc_spvc_compile_options_set_webgpu_to_vulkan(options, true);

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_glsl(
      compiler, kWebGPUShaderBinary,
      sizeof(kWebGPUShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_compiler_release(compiler);
}

TEST(Compile, Hlsl) {
  shaderc_spvc_compiler_t compiler = shaderc_spvc_compiler_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compile_options_set_source_env(
      options, shaderc_target_env_webgpu, shaderc_env_version_webgpu);
  shaderc_spvc_compile_options_set_webgpu_to_vulkan(options, true);

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_hlsl(
      compiler, kWebGPUShaderBinary,
      sizeof(kWebGPUShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_compiler_release(compiler);
}

TEST(Compile, Msl) {
  shaderc_spvc_compiler_t compiler = shaderc_spvc_compiler_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compile_options_set_source_env(
      options, shaderc_target_env_webgpu, shaderc_env_version_webgpu);
  shaderc_spvc_compile_options_set_webgpu_to_vulkan(options, true);

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_msl(
      compiler, kWebGPUShaderBinary,
      sizeof(kWebGPUShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_compiler_release(compiler);
}

}  // anonymous namespace
