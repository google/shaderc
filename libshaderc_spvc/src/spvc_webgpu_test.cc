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
#include "spvc/spvc.h"
#include "spvc_private.h"

namespace {

TEST(Compile, ValidShaderIntoGlslPasses) {
  shaderc_spvc_compiler_t compiler = shaderc_spvc_compiler_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compile_options_set_source_env(
      options, shaderc_target_env_webgpu, shaderc_env_version_webgpu);
  shaderc_spvc_compile_options_set_target_env(
      options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_glsl(
      compiler, kWebGPUShaderBinary,
      sizeof(kWebGPUShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_NE(result->compiler.get(), nullptr);

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_compiler_release(compiler);
}

TEST(Compile, ValidShaderIntoHlslPasses) {
  shaderc_spvc_compiler_t compiler = shaderc_spvc_compiler_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compile_options_set_source_env(
      options, shaderc_target_env_webgpu, shaderc_env_version_webgpu);
  shaderc_spvc_compile_options_set_target_env(
      options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_hlsl(
      compiler, kWebGPUShaderBinary,
      sizeof(kWebGPUShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_NE(result->compiler.get(), nullptr);

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_compiler_release(compiler);
}

TEST(Compile, ValidShaderIntoMslPasses) {
  shaderc_spvc_compiler_t compiler = shaderc_spvc_compiler_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compile_options_set_source_env(
      options, shaderc_target_env_webgpu, shaderc_env_version_webgpu);
  shaderc_spvc_compile_options_set_target_env(
      options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_msl(
      compiler, kWebGPUShaderBinary,
      sizeof(kWebGPUShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_NE(result->compiler.get(), nullptr);

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_compiler_release(compiler);
}

TEST(Compile, ValidShaderIntoVulkanPasses) {
  shaderc_spvc_compiler_t compiler = shaderc_spvc_compiler_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compile_options_set_source_env(
      options, shaderc_target_env_webgpu, shaderc_env_version_webgpu);
  shaderc_spvc_compile_options_set_target_env(
      options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_vulkan(
      compiler, kWebGPUShaderBinary,
      sizeof(kWebGPUShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_NE(result->compiler.get(), nullptr);

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_compiler_release(compiler);
}

}  // anonymous namespace
