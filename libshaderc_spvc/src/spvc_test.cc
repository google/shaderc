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

#include <gtest/gtest.h>
#include <thread>

#include "common_shaders_for_test.h"
#include "shaderc/spvc.h"

namespace {

TEST(Init, MultipleCalls) {
  spvc_compiler_t compiler1, compiler2, compiler3;
  EXPECT_NE(nullptr, compiler1 = spvc_compiler_initialize());
  EXPECT_NE(nullptr, compiler2 = spvc_compiler_initialize());
  EXPECT_NE(nullptr, compiler3 = spvc_compiler_initialize());
  spvc_compiler_release(compiler1);
  spvc_compiler_release(compiler2);
  spvc_compiler_release(compiler3);
}

#ifndef SHADERC_DISABLE_THREADED_TESTS
TEST(Init, MultipleThreadsCalling) {
  spvc_compiler_t compiler1, compiler2, compiler3;
  std::thread t1([&compiler1]() { compiler1 = spvc_compiler_initialize(); });
  std::thread t2([&compiler2]() { compiler2 = spvc_compiler_initialize(); });
  std::thread t3([&compiler3]() { compiler3 = spvc_compiler_initialize(); });
  t1.join();
  t2.join();
  t3.join();
  EXPECT_NE(nullptr, compiler1);
  EXPECT_NE(nullptr, compiler2);
  EXPECT_NE(nullptr, compiler3);
  spvc_compiler_release(compiler1);
  spvc_compiler_release(compiler2);
  spvc_compiler_release(compiler3);
}
#endif

TEST(Compile, Test1) {
  spvc_compiler_t compiler;
  spvc_compile_options_t options;

  compiler = spvc_compiler_initialize();
  options = spvc_compile_options_initialize();

  spvc_compilation_result_t result = spvc_compile_into_glsl(compiler, kShader1, sizeof(kShader1)/sizeof(uint32_t), options);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(spvc_compilation_status_success, spvc_result_get_compilation_status(result));

  spvc_result_release(result);
  spvc_compile_options_release(options);
  spvc_compiler_release(compiler);
}

}  // anonymous namespace
