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
  shaderc_spvc_state_t state1, state2, state3;
  EXPECT_NE(nullptr, state1 = shaderc_spvc_state_initialize());
  EXPECT_NE(nullptr, state2 = shaderc_spvc_state_initialize());
  EXPECT_NE(nullptr, state3 = shaderc_spvc_state_initialize());
  shaderc_spvc_state_release(state1);
  shaderc_spvc_state_release(state2);
  shaderc_spvc_state_release(state3);
}

#ifndef SHADERC_DISABLE_THREADED_TESTS
TEST(Init, MultipleThreadsCalling) {
  shaderc_spvc_state_t state1, state2, state3;
  std::thread t1([&state1]() { state1 = shaderc_spvc_state_initialize(); });
  std::thread t2([&state2]() { state2 = shaderc_spvc_state_initialize(); });
  std::thread t3([&state3]() { state3 = shaderc_spvc_state_initialize(); });
  t1.join();
  t2.join();
  t3.join();
  EXPECT_NE(nullptr, state1);
  EXPECT_NE(nullptr, state2);
  EXPECT_NE(nullptr, state3);
  shaderc_spvc_state_release(state1);
  shaderc_spvc_state_release(state2);
  shaderc_spvc_state_release(state3);
}
#endif

TEST(Compile, ValidShaderIntoGlslPasses) {
  shaderc_spvc_state_t state = shaderc_spvc_state_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_glsl(
      state, kSmokeShaderBinary, sizeof(kSmokeShaderBinary) / sizeof(uint32_t),
      options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_NE(state->cross_compiler.get(), nullptr);

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_state_release(state);
}

TEST(Compile, InvalidShaderIntoGlslPasses) {
  shaderc_spvc_state_t state = shaderc_spvc_state_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_glsl(
      state, kInvalidShaderBinary,
      sizeof(kInvalidShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_NE(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_EQ(state->cross_compiler.get(), nullptr);

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_state_release(state);
}

TEST(Compile, ValidShaderIntoHlslPasses) {
  shaderc_spvc_state_t state = shaderc_spvc_state_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_hlsl(
      state, kSmokeShaderBinary, sizeof(kSmokeShaderBinary) / sizeof(uint32_t),
      options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_NE(state->cross_compiler.get(), nullptr);

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_state_release(state);
}

TEST(Compile, InvalidShaderIntoHlslPasses) {
  shaderc_spvc_state_t state = shaderc_spvc_state_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_hlsl(
      state, kInvalidShaderBinary,
      sizeof(kInvalidShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_NE(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_EQ(state->cross_compiler.get(), nullptr);

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_state_release(state);
}

TEST(Compile, ValidShaderIntoMslPasses) {
  shaderc_spvc_state_t state = shaderc_spvc_state_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_msl(
      state, kSmokeShaderBinary, sizeof(kSmokeShaderBinary) / sizeof(uint32_t),
      options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_NE(state->cross_compiler.get(), nullptr);

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_state_release(state);
}

TEST(Compile, InvalidShaderIntoMslPasses) {
  shaderc_spvc_state_t state = shaderc_spvc_state_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_msl(
      state, kInvalidShaderBinary,
      sizeof(kInvalidShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_NE(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_EQ(state->cross_compiler.get(), nullptr);

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_state_release(state);
}

TEST(Compile, ValidShaderIntoVulkanPasses) {
  shaderc_spvc_state_t state = shaderc_spvc_state_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_vulkan(
      state, kSmokeShaderBinary, sizeof(kSmokeShaderBinary) / sizeof(uint32_t),
      options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_NE(state->cross_compiler.get(), nullptr);

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_state_release(state);
}

TEST(Compile, InvalidShaderIntoVulkanPasses) {
  shaderc_spvc_state_t state = shaderc_spvc_state_initialize();
  shaderc_spvc_compile_options_t options =
      shaderc_spvc_compile_options_initialize();

  shaderc_spvc_compilation_result_t result = shaderc_spvc_compile_into_vulkan(
      state, kInvalidShaderBinary,
      sizeof(kInvalidShaderBinary) / sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_NE(shaderc_compilation_status_success,
            shaderc_spvc_result_get_status(result));
  EXPECT_EQ(state->cross_compiler.get(), nullptr);

  shaderc_spvc_result_release(result);
  shaderc_spvc_compile_options_release(options);
  shaderc_spvc_state_release(state);
}

}  // namespace
