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

#include "spvc/spvc.h"

#include <gtest/gtest.h>

#include <thread>

#include "common_shaders_for_test.h"
#include "spvc_private.h"

namespace {

class CompileTest : public testing::Test {
 public:
  void SetUp() override {
    context_ = shaderc_spvc_context_create();
    options_ = shaderc_spvc_compile_options_create();
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

TEST(Init, MultipleCalls) {
  shaderc_spvc_context_t context1, context2, context3;
  EXPECT_NE(nullptr, context1 = shaderc_spvc_context_create());
  EXPECT_NE(nullptr, context2 = shaderc_spvc_context_create());
  EXPECT_NE(nullptr, context3 = shaderc_spvc_context_create());
  shaderc_spvc_context_destroy(context1);
  shaderc_spvc_context_destroy(context2);
  shaderc_spvc_context_destroy(context3);
}

#ifndef SHADERC_DISABLE_THREADED_TESTS
TEST(Init, MultipleThreadsCalling) {
  shaderc_spvc_context_t context1, context2, context3;
  std::thread t1([&context1]() { context1 = shaderc_spvc_context_create(); });
  std::thread t2([&context2]() { context2 = shaderc_spvc_context_create(); });
  std::thread t3([&context3]() { context3 = shaderc_spvc_context_create(); });
  t1.join();
  t2.join();
  t3.join();
  EXPECT_NE(nullptr, context1);
  EXPECT_NE(nullptr, context2);
  EXPECT_NE(nullptr, context3);
  shaderc_spvc_context_destroy(context1);
  shaderc_spvc_context_destroy(context2);
  shaderc_spvc_context_destroy(context3);
}
#endif

TEST_F(CompileTest, ValidShaderIntoGlslPasses) {
  {
    shaderc_spvc_status status = shaderc_spvc_initialize_for_glsl(
        context_, kSmokeShaderBinary,
        sizeof(kSmokeShaderBinary) / sizeof(uint32_t), options_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
    EXPECT_NE(context_->cross_compiler.get(), nullptr);
  }
  {
    shaderc_spvc_status status = shaderc_spvc_compile_shader(context_, result_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
  }
}

TEST_F(CompileTest, InvalidShaderIntoGlslPasses) {
  shaderc_spvc_status status = shaderc_spvc_initialize_for_glsl(
      context_, kInvalidShaderBinary,
      sizeof(kInvalidShaderBinary) / sizeof(uint32_t), options_);
  EXPECT_NE(shaderc_spvc_status_success, status);
  EXPECT_EQ(context_->cross_compiler.get(), nullptr);
}

TEST_F(CompileTest, ValidShaderIntoHlslPasses) {
  {
    shaderc_spvc_status status = shaderc_spvc_initialize_for_hlsl(
        context_, kSmokeShaderBinary,
        sizeof(kSmokeShaderBinary) / sizeof(uint32_t), options_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
    EXPECT_NE(context_->cross_compiler.get(), nullptr);
  }
  {
    shaderc_spvc_status status = shaderc_spvc_compile_shader(context_, result_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
  }
}

TEST_F(CompileTest, InvalidShaderIntoHlslPasses) {
  shaderc_spvc_status status = shaderc_spvc_initialize_for_hlsl(
      context_, kInvalidShaderBinary,
      sizeof(kInvalidShaderBinary) / sizeof(uint32_t), options_);
  EXPECT_NE(shaderc_spvc_status_success, status);
  EXPECT_EQ(context_->cross_compiler.get(), nullptr);
}

TEST_F(CompileTest, ValidShaderIntoMslPasses) {
  {
    shaderc_spvc_status status = shaderc_spvc_initialize_for_msl(
        context_, kSmokeShaderBinary,
        sizeof(kSmokeShaderBinary) / sizeof(uint32_t), options_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
    EXPECT_NE(context_->cross_compiler.get(), nullptr);
  }
  {
    shaderc_spvc_status status = shaderc_spvc_compile_shader(context_, result_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
  }
}

TEST_F(CompileTest, InvalidShaderIntoMslPasses) {
  shaderc_spvc_status status = shaderc_spvc_initialize_for_msl(
      context_, kInvalidShaderBinary,
      sizeof(kInvalidShaderBinary) / sizeof(uint32_t), options_);
  EXPECT_NE(shaderc_spvc_status_success, status);
  EXPECT_EQ(context_->cross_compiler.get(), nullptr);
}

TEST_F(CompileTest, ValidShaderIntoVulkanPasses) {
  {
    shaderc_spvc_status status = shaderc_spvc_initialize_for_vulkan(
        context_, kSmokeShaderBinary,
        sizeof(kSmokeShaderBinary) / sizeof(uint32_t), options_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
    EXPECT_NE(context_->cross_compiler.get(), nullptr);
  }
  {
    shaderc_spvc_status status = shaderc_spvc_compile_shader(context_, result_);
    EXPECT_EQ(shaderc_spvc_status_success, status);
  }
}

TEST_F(CompileTest, InvalidShaderIntoVulkanPasses) {
  shaderc_spvc_status status = shaderc_spvc_initialize_for_vulkan(
      context_, kInvalidShaderBinary,
      sizeof(kInvalidShaderBinary) / sizeof(uint32_t), options_);
  EXPECT_NE(shaderc_spvc_status_success, status);
  EXPECT_EQ(context_->cross_compiler.get(), nullptr);
}

}  // namespace
