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

TEST(Compile, ValidShaderIntoGlslPasses) {
  shaderc_spvc_context_t context = shaderc_spvc_context_create();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_create();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_glsl(
      context, kSmokeShaderBinary,
      sizeof(kSmokeShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_NE(context->cross_compiler.get(), nullptr);

  shaderc_spvc_result_destroy(result);
  shaderc_spvc_compile_options_destroy(options);
  shaderc_spvc_context_destroy(context);
}

TEST(Compile, InvalidShaderIntoGlslPasses) {
  shaderc_spvc_context_t context = shaderc_spvc_context_create();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_create();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_glsl(
      context, kInvalidShaderBinary,
      sizeof(kInvalidShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_NE(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_EQ(context->cross_compiler.get(), nullptr);

  shaderc_spvc_result_destroy(result);
  shaderc_spvc_compile_options_destroy(options);
  shaderc_spvc_context_destroy(context);
}

TEST(Compile, ValidShaderIntoHlslPasses) {
  shaderc_spvc_context_t context = shaderc_spvc_context_create();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_create();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_hlsl(
      context, kSmokeShaderBinary,
      sizeof(kSmokeShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_NE(context->cross_compiler.get(), nullptr);

  shaderc_spvc_result_destroy(result);
  shaderc_spvc_compile_options_destroy(options);
  shaderc_spvc_context_destroy(context);
}

TEST(Compile, InvalidShaderIntoHlslPasses) {
  shaderc_spvc_context_t context = shaderc_spvc_context_create();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_create();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_hlsl(
      context, kInvalidShaderBinary,
      sizeof(kInvalidShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_NE(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_EQ(context->cross_compiler.get(), nullptr);

  shaderc_spvc_result_destroy(result);
  shaderc_spvc_compile_options_destroy(options);
  shaderc_spvc_context_destroy(context);
}

TEST(Compile, ValidShaderIntoMslPasses) {
  shaderc_spvc_context_t context = shaderc_spvc_context_create();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_create();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_msl(
      context, kSmokeShaderBinary,
      sizeof(kSmokeShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_NE(context->cross_compiler.get(), nullptr);

  shaderc_spvc_result_destroy(result);
  shaderc_spvc_compile_options_destroy(options);
  shaderc_spvc_context_destroy(context);
}

TEST(Compile, InvalidShaderIntoMslPasses) {
  shaderc_spvc_context_t context = shaderc_spvc_context_create();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_create();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_msl(
      context, kInvalidShaderBinary,
      sizeof(kInvalidShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_NE(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_EQ(context->cross_compiler.get(), nullptr);

  shaderc_spvc_result_destroy(result);
  shaderc_spvc_compile_options_destroy(options);
  shaderc_spvc_context_destroy(context);
}

TEST(Compile, ValidShaderIntoVulkanPasses) {
  shaderc_spvc_context_t context = shaderc_spvc_context_create();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_create();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_vulkan(
      context, kSmokeShaderBinary,
      sizeof(kSmokeShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_NE(context->cross_compiler.get(), nullptr);

  shaderc_spvc_result_destroy(result);
  shaderc_spvc_compile_options_destroy(options);
  shaderc_spvc_context_destroy(context);
}

TEST(Compile, InvalidShaderIntoVulkanPasses) {
  shaderc_spvc_context_t context = shaderc_spvc_context_create();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_create();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_vulkan(
      context, kInvalidShaderBinary,
      sizeof(kInvalidShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_NE(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_EQ(context->cross_compiler.get(), nullptr);

  shaderc_spvc_result_destroy(result);
  shaderc_spvc_compile_options_destroy(options);
  shaderc_spvc_context_destroy(context);
}

}  // namespace
