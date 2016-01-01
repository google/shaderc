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

#include "SPIRV/spirv.hpp"

namespace {

using testing::Each;
using testing::HasSubstr;

const char kMinimalShader[] = "void main(){}";
const std::string kMinimalShaderWithMacro =
    "#define E main\n"
    "void E(){}\n";

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

TEST(CppInterface, DisassemblyOption) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;
  options.SetDisassemblyMode();
  shaderc::SpvModule result = compiler.CompileGlslToSpv(
      kMinimalShader, shaderc_glsl_vertex_shader, options);
  EXPECT_TRUE(result.GetSuccess());
  // This should work with both the glslang native disassembly format and the
  // SPIR-V Tools assembly format.
  EXPECT_THAT(result.GetData(), HasSubstr("Capability Shader"));
  EXPECT_THAT(result.GetData(), HasSubstr("MemoryModel"));

  shaderc::CompileOptions cloned_options(options);
  shaderc::SpvModule result_from_cloned_options = compiler.CompileGlslToSpv(
      kMinimalShader, shaderc_glsl_vertex_shader, cloned_options);
  EXPECT_TRUE(result_from_cloned_options.GetSuccess());
  // The mode should be carried into any clone of the original option object.
  EXPECT_THAT(result_from_cloned_options.GetData(),
              HasSubstr("Capability Shader"));
  EXPECT_THAT(result_from_cloned_options.GetData(), HasSubstr("MemoryModel"));
}

TEST(CppInterface, PreprocessingOnlyOption) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;
  options.SetPreprocessingOnlyMode();
  shaderc::SpvModule result = compiler.CompileGlslToSpv(
      kMinimalShaderWithMacro, shaderc_glsl_vertex_shader, options);
  EXPECT_TRUE(result.GetSuccess());
  EXPECT_THAT(result.GetData(), HasSubstr("void main(){ }"));

  const std::string kMinimalShaderCloneOption =
      "#define E_CLONE_OPTION main\n"
      "void E_CLONE_OPTION(){}\n";
  shaderc::CompileOptions cloned_options(options);
  shaderc::SpvModule result_from_cloned_options = compiler.CompileGlslToSpv(
      kMinimalShaderCloneOption, shaderc_glsl_vertex_shader, cloned_options);
  EXPECT_TRUE(result_from_cloned_options.GetSuccess());
  EXPECT_THAT(result_from_cloned_options.GetData(),
              HasSubstr("void main(){ }"));
}

TEST(CppInterface, PreprocessingOnlyModeFirstOverridesDisassemblyMode) {
  shaderc::Compiler compiler;
  // Sets preprocessing only mode first, then sets disassembly mode.
  // Preprocessing only mode should override disassembly mode.
  shaderc::CompileOptions options_preprocessing_mode_first;
  options_preprocessing_mode_first.SetPreprocessingOnlyMode();
  options_preprocessing_mode_first.SetDisassemblyMode();
  shaderc::SpvModule result_preprocessing_mode_first =
      compiler.CompileGlslToSpv(kMinimalShaderWithMacro,
                                shaderc_glsl_vertex_shader,
                                options_preprocessing_mode_first);
  EXPECT_TRUE(result_preprocessing_mode_first.GetSuccess());
  EXPECT_THAT(result_preprocessing_mode_first.GetData(),
              HasSubstr("void main(){ }"));
}

TEST(CppInterface, PreprocessingOnlyModeSecondOverridesDisassemblyMode) {
  shaderc::Compiler compiler;
  // Sets disassembly mode first, then preprocessing only mode.
  // Preprocessing only mode should still override disassembly mode.
  shaderc::CompileOptions options_disassembly_mode_first;
  options_disassembly_mode_first.SetDisassemblyMode();
  options_disassembly_mode_first.SetPreprocessingOnlyMode();
  shaderc::SpvModule result_disassembly_mode_first = compiler.CompileGlslToSpv(
      kMinimalShaderWithMacro, shaderc_glsl_vertex_shader,
      options_disassembly_mode_first);
  EXPECT_TRUE(result_disassembly_mode_first.GetSuccess());
  EXPECT_THAT(result_disassembly_mode_first.GetData(),
              HasSubstr("void main(){ }"));
}

TEST(CppInterface, WarningsOnLine) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;
  // By default the compiler will emit a warning on line 2 complaining
  // 'float' as a deprecated attribute in version 130.
  const std::string kDeprecatedAttributeShader =
      "#version 130\n"
      "attribute float x;\n"
      "void main() {}\n";
  shaderc::SpvModule result_suppress_warnings_on_line =
      compiler.CompileGlslToSpv(kDeprecatedAttributeShader,
                                shaderc_glsl_vertex_shader, options);
  EXPECT_TRUE(result_suppress_warnings_on_line.GetSuccess());
  EXPECT_THAT(
      result_suppress_warnings_on_line.GetErrorMessage().c_str(),
      HasSubstr(":2: warning: attribute deprecated in version 130; may be "
                "removed in future release\n"));
}

TEST(CppInterface, SuppressWarningsOnLine) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options_suppress_warnings;
  // Sets the compiler to suppress warnings, so that the deprecated attribute
  // warning won't be emitted.
  options_suppress_warnings.SetSuppressWarnings();
  const std::string kDeprecatedAttributeShader =
      "#version 130\n"
      "attribute float x;\n"
      "void main() {}\n";
  shaderc::SpvModule result_suppress_warnings_on_line =
      compiler.CompileGlslToSpv(kDeprecatedAttributeShader,
                                shaderc_glsl_vertex_shader,
                                options_suppress_warnings);
  EXPECT_TRUE(result_suppress_warnings_on_line.GetSuccess());
  EXPECT_STREQ(result_suppress_warnings_on_line.GetErrorMessage().c_str(), "");
}

TEST(CppInterface, GlobalWarnings) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;
  // By default the compiler will emit a warning as version 550 is an unknown
  // version.
  const std::string kMinimalUnknownVersionShader =
      "#version 550\n"
      "void main() {}\n";
  shaderc::SpvModule result_suppress_warnings_on_line =
      compiler.CompileGlslToSpv(kMinimalUnknownVersionShader,
                                shaderc_glsl_vertex_shader, options);
  EXPECT_TRUE(result_suppress_warnings_on_line.GetSuccess());
  EXPECT_THAT(result_suppress_warnings_on_line.GetErrorMessage().c_str(),
              HasSubstr("warning: version 550 is unknown.\n"));
}

TEST(CppInterface, SuppressGlobalWarnings) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options_suppress_warnings;
  // Sets the compiler to suppress warnings, so that the unknown version warning
  // won't be emitted.
  options_suppress_warnings.SetSuppressWarnings();
  const std::string kMinimalUnknownVersionShader =
      "#version 550\n"
      "void main() {}\n";
  shaderc::SpvModule result_suppress_warnings_on_line =
      compiler.CompileGlslToSpv(kMinimalUnknownVersionShader,
                                shaderc_glsl_vertex_shader,
                                options_suppress_warnings);
  EXPECT_TRUE(result_suppress_warnings_on_line.GetSuccess());
  EXPECT_STREQ(result_suppress_warnings_on_line.GetErrorMessage().c_str(), "");
}

TEST(CppInterface, TargetEnvCompileOptions) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;

  // Test shader compilation which requires opengl compatibility environment
  options.SetTargetEnvironment(shaderc_target_env_opengl_compat, 0);
  const std::string kGlslShader =
    R"(#version 100
       uniform highp sampler2D tex;
       void main() {
         gl_FragColor = texture2D(tex, vec2(0.0,0.0));
       }
  )";

  EXPECT_TRUE(compiler.CompileGlslToSpv(kGlslShader,
                                        shaderc_glsl_fragment_shader,
                                        options)
                  .GetSuccess());
}

}  // anonymous namespace
