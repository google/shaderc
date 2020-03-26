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

class CompileTest : public testing::Test {
 public:
  void SetUp() override {
    context_ = shaderc_spvc_context_create();
    options_ = shaderc_spvc_compile_options_create(
        shaderc_spvc_spv_env_webgpu_0, shaderc_spvc_spv_env_vulkan_1_1);
    result_ = shaderc_spvc_result_create();
  }

  void TearDown() override {
    shaderc_spvc_context_destroy(context_);
    shaderc_spvc_compile_options_destroy(options_);
    shaderc_spvc_result_destroy(result_);
  }

  shaderc_spvc_context_t context_;
  shaderc_spvc_compile_options_t options_;
  shaderc_spvc_compilation_result_t result_;
};

TEST_F(CompileTest, ValidShaderIntoGlslPasses) {
  {
    shaderc_spvc_status status = shaderc_spvc_initialize_for_glsl(
        context_, kWebGPUShaderBinary,
        sizeof(kWebGPUShaderBinary) / sizeof(uint32_t), options_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
    EXPECT_NE(context_->cross_compiler.get(), nullptr);
  }
  {
    shaderc_spvc_status status =
        shaderc_spvc_compile_shader(context_, result_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
  }
}

TEST_F(CompileTest, ValidShaderIntoHlslPasses) {
  {
    shaderc_spvc_status status = shaderc_spvc_initialize_for_hlsl(
        context_, kWebGPUShaderBinary,
        sizeof(kWebGPUShaderBinary) / sizeof(uint32_t), options_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
    EXPECT_NE(context_->cross_compiler.get(), nullptr);
  }
  {
    shaderc_spvc_status status =
        shaderc_spvc_compile_shader(context_, result_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
  }
}

TEST_F(CompileTest, ValidShaderIntoMslPasses) {
  {
    shaderc_spvc_status status = shaderc_spvc_initialize_for_msl(
        context_, kWebGPUShaderBinary,
        sizeof(kWebGPUShaderBinary) / sizeof(uint32_t), options_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
    EXPECT_NE(context_->cross_compiler.get(), nullptr);
  }
  {
    shaderc_spvc_status status =
        shaderc_spvc_compile_shader(context_, result_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
  }
}

TEST_F(CompileTest, ValidShaderIntoVulkanPasses) {
  {
    shaderc_spvc_status status = shaderc_spvc_initialize_for_vulkan(
        context_, kWebGPUShaderBinary,
        sizeof(kWebGPUShaderBinary) / sizeof(uint32_t), options_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
    EXPECT_NE(context_->cross_compiler.get(), nullptr);
  }
  {
    shaderc_spvc_status status =
        shaderc_spvc_compile_shader(context_, result_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
  }
}

}  // anonymous namespace
