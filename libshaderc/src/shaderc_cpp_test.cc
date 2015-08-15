// Copyright 2015 The Shaderc Authors. All rights reserved.
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

#include "shaderc.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

#include "SPIRV/spirv.h"

namespace {

using testing::Each;
using testing::HasSubstr;

const char kMinimalShader[] = "void main(){}";

TEST(CppInterface, MultipleCalls) {
  shaderc::Compiler compiler1, compiler2, compiler3;
  EXPECT_TRUE(compiler1.IsValid());
  EXPECT_TRUE(compiler2.IsValid());
  EXPECT_TRUE(compiler3.IsValid());
}

TEST(CppInterface, MultipleThreadsInitializing) {
  std::unique_ptr<shaderc::Compiler> compiler1;
  std::unique_ptr<shaderc::Compiler> compiler2;
  std::unique_ptr<shaderc::Compiler> compiler3;
  std::thread t1([&compiler1]() {
    compiler1 = std::unique_ptr<shaderc::Compiler>(new shaderc::Compiler());
  });
  std::thread t2([&compiler2]() {
    compiler2 = std::unique_ptr<shaderc::Compiler>(new shaderc::Compiler());
  });
  std::thread t3([&compiler3]() {
    compiler3 = std::unique_ptr<shaderc::Compiler>(new shaderc::Compiler());
  });
  t1.join();
  t2.join();
  t3.join();
  EXPECT_TRUE(compiler1->IsValid());
  EXPECT_TRUE(compiler2->IsValid());
  EXPECT_TRUE(compiler3->IsValid());
}

// Compiles a shader and returns true on success, false on failure.
bool CompilationSuccess(const shaderc::Compiler& compiler,
                        const std::string& shader, shaderc_shader_kind kind) {
  return compiler.CompileGlslToSpv(shader.c_str(), shader.length(), kind)
      .GetSuccess();
}

// Compiles a shader and returns true if the result is valid SPIR-V.
bool CompilesToValidSpv(const shaderc::Compiler& compiler,
                        const std::string& shader, shaderc_shader_kind kind) {
  shaderc::SpvModule result = compiler.CompileGlslToSpv(shader, kind);
  if (!result.GetSuccess()) return false;
  size_t length = result.GetLength();
  if (length < 20) return false;
  const uint32_t* bytes =
      static_cast<const uint32_t*>(static_cast<const void*>(result.GetData()));
  return bytes[0] == spv::MagicNumber;
}

// Compiles a shader with the given options and returns true if the result is
// valid SPIR-V.
bool CompilesToValidSpv(const shaderc::Compiler& compiler,
                        const std::string& shader, shaderc_shader_kind kind,
                        const shaderc::CompileOptions& options) {
  shaderc::SpvModule result = compiler.CompileGlslToSpv(shader, kind, options);
  if (!result.GetSuccess()) return false;
  size_t length = result.GetLength();
  if (length < 20) return false;
  const uint32_t* bytes =
      static_cast<const uint32_t*>(static_cast<const void*>(result.GetData()));
  return bytes[0] == spv::MagicNumber;
}

TEST(CppInterface, CompilerMoves) {
  shaderc::Compiler compiler;
  ASSERT_TRUE(compiler.IsValid());
  shaderc::Compiler compiler2(std::move(compiler));
  ASSERT_FALSE(compiler.IsValid());
  ASSERT_TRUE(compiler2.IsValid());
}

TEST(CppInterface, EmptyString) {
  shaderc::Compiler compiler;
  ASSERT_TRUE(compiler.IsValid());
  EXPECT_FALSE(CompilationSuccess(compiler, "", shaderc_glsl_vertex_shader));
  EXPECT_FALSE(CompilationSuccess(compiler, "", shaderc_glsl_fragment_shader));
}

TEST(CppInterface, ModuleMoves) {
  shaderc::Compiler compiler;
  ASSERT_TRUE(compiler.IsValid());
  shaderc::SpvModule result =
      compiler.CompileGlslToSpv(kMinimalShader, shaderc_glsl_vertex_shader);
  EXPECT_TRUE(result.GetSuccess());
  shaderc::SpvModule result2(std::move(result));
  EXPECT_FALSE(result.GetSuccess());
  EXPECT_TRUE(result2.GetSuccess());
}

TEST(CppInterface, GarbageString) {
  shaderc::Compiler compiler;
  ASSERT_TRUE(compiler.IsValid());
  EXPECT_FALSE(
      CompilationSuccess(compiler, "jfalkds", shaderc_glsl_vertex_shader));
  EXPECT_FALSE(
      CompilationSuccess(compiler, "jfalkds", shaderc_glsl_fragment_shader));
}

TEST(CppInterface, MinimalShader) {
  shaderc::Compiler compiler;
  ASSERT_TRUE(compiler.IsValid());
  EXPECT_TRUE(
      CompilesToValidSpv(compiler, kMinimalShader, shaderc_glsl_vertex_shader));
  EXPECT_TRUE(CompilesToValidSpv(compiler, kMinimalShader,
                                 shaderc_glsl_fragment_shader));
}

TEST(CppInterface, BasicOptions) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;
  ASSERT_TRUE(compiler.IsValid());
  EXPECT_TRUE(CompilesToValidSpv(compiler, kMinimalShader,
                                 shaderc_glsl_vertex_shader, options));
  EXPECT_TRUE(CompilesToValidSpv(compiler, kMinimalShader,
                                 shaderc_glsl_fragment_shader, options));
}

TEST(CppInterface, CopiedOptions) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;
  ASSERT_TRUE(compiler.IsValid());
  EXPECT_TRUE(CompilesToValidSpv(compiler, kMinimalShader,
                                 shaderc_glsl_vertex_shader, options));
  shaderc::CompileOptions copied_options(options);
  EXPECT_TRUE(CompilesToValidSpv(compiler, kMinimalShader,
                                 shaderc_glsl_fragment_shader, copied_options));
}

TEST(CppInterface, MovedOptions) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;
  ASSERT_TRUE(compiler.IsValid());
  EXPECT_TRUE(CompilesToValidSpv(compiler, kMinimalShader,
                                 shaderc_glsl_vertex_shader, options));
  shaderc::CompileOptions copied_options(std::move(options));
  EXPECT_TRUE(CompilesToValidSpv(compiler, kMinimalShader,
                                 shaderc_glsl_fragment_shader, copied_options));
}

TEST(CppInterface, StdAndCString) {
  shaderc::Compiler compiler;
  ASSERT_TRUE(compiler.IsValid());
  shaderc::SpvModule result1 = compiler.CompileGlslToSpv(
      kMinimalShader, strlen(kMinimalShader), shaderc_glsl_fragment_shader);
  shaderc::SpvModule result2 = compiler.CompileGlslToSpv(
      std::string(kMinimalShader), shaderc_glsl_fragment_shader);
  EXPECT_TRUE(result1.GetSuccess());
  EXPECT_TRUE(result2.GetSuccess());
  EXPECT_EQ(result1.GetLength(), result2.GetLength());
  EXPECT_EQ(std::vector<char>(result1.GetData(),
                              result1.GetData() + result1.GetLength()),
            std::vector<char>(result2.GetData(),
                              result2.GetData() + result2.GetLength()));
}

TEST(CppInterface, ErrorsReported) {
  shaderc::Compiler compiler;
  ASSERT_TRUE(compiler.IsValid());
  shaderc::SpvModule result = compiler.CompileGlslToSpv(
      "int f(){return wrongname;}", shaderc_glsl_vertex_shader);
  ASSERT_FALSE(result.GetSuccess());
  EXPECT_THAT(result.GetErrorMessage(), HasSubstr("wrongname"));
}

TEST(CppInterface, MultipleThreadsCalling) {
  shaderc::Compiler compiler;
  ASSERT_TRUE(compiler.IsValid());
  bool results[10];
  std::vector<std::thread> threads;
  for (auto& r : results) {
    threads.emplace_back([&compiler, &r]() {
      r = CompilationSuccess(compiler, kMinimalShader,
                             shaderc_glsl_vertex_shader);
    });
  }
  for (auto& t : threads) {
    t.join();
  }
  EXPECT_THAT(results, Each(true));
}

TEST(CppInterface, AccessorsOnNullModule) {
  shaderc::SpvModule result(nullptr);
  EXPECT_FALSE(result.GetSuccess());
  EXPECT_EQ(std::string(), result.GetErrorMessage());
  EXPECT_EQ(std::string(), result.GetData());
  EXPECT_EQ(0u, result.GetLength());
}

TEST(CppInterface, MacroCompileOptions) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;
  options.AddMacroDefinition("E", "main");
  const std::string kMinimalExpandedShader = "void E(){}";
  const std::string kMinimalDoubleExpandedShader = "F E(){}";
  EXPECT_TRUE(compiler.CompileGlslToSpv(kMinimalExpandedShader,
                                        shaderc_glsl_vertex_shader, options)
                  .GetSuccess());

  shaderc::CompileOptions cloned_options(options);
  // The simplest should still compile with the cloned options.
  EXPECT_TRUE(compiler.CompileGlslToSpv(kMinimalExpandedShader,
                                        shaderc_glsl_vertex_shader,
                                        cloned_options)
                  .GetSuccess());

  EXPECT_FALSE(compiler.CompileGlslToSpv(kMinimalDoubleExpandedShader,
                                         shaderc_glsl_vertex_shader,
                                         cloned_options)
                   .GetSuccess());

  cloned_options.AddMacroDefinition("F", "void");
  // This should still not work with the original options.
  EXPECT_FALSE(compiler.CompileGlslToSpv(kMinimalDoubleExpandedShader,
                                         shaderc_glsl_vertex_shader, options)
                   .GetSuccess());
  // This should work with the cloned options that have the additional
  // parameter.
  EXPECT_TRUE(compiler.CompileGlslToSpv(kMinimalDoubleExpandedShader,
                                        shaderc_glsl_vertex_shader,
                                        cloned_options)
                  .GetSuccess());
}

}  // anonymous namespace
