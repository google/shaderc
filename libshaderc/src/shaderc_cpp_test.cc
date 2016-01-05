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

using shaderc::CompileOptions;
using testing::Each;
using testing::HasSubstr;

const char kMinimalShader[] = "void main(){}";
const std::string kMinimalShaderWithMacro =
    "#define E main\n"
    "void E(){}\n";

// By default the compiler will emit a warning on line 2 complaining
// that 'float' is a deprecated attribute in version 130.
const std::string kDeprecatedAttributeShader =
    "#version 130\n"
    "attribute float x;\n"
    "void main() {}\n";

// By default the compiler will emit a warning as version 550 is an unknown
// version.
const std::string kMinimalUnknownVersionShader =
    "#version 550\n"
    "void main() {}\n";

class CppInterface : public testing::Test {
 protected:
  // Compiles a shader and returns true on success, false on failure.
  bool CompilationSuccess(const std::string& shader,
                          shaderc_shader_kind kind) const {
    return compiler_.CompileGlslToSpv(shader.c_str(), shader.length(), kind)
        .GetSuccess();
  }

  // Compiles a shader with options and returns true on success, false on
  // failure.
  bool CompilationSuccess(const std::string& shader, shaderc_shader_kind kind,
                          const CompileOptions& options) const {
    return compiler_
        .CompileGlslToSpv(shader.c_str(), shader.length(), kind, options)
        .GetSuccess();
  }

  // Compiles a shader and returns true if the result is valid SPIR-V.
  bool CompilesToValidSpv(const std::string& shader,
                          shaderc_shader_kind kind) const {
    return IsValidSpv(compiler_.CompileGlslToSpv(shader, kind));
  }

  // Compiles a shader with options and returns true if the result is valid
  // SPIR-V.
  bool CompilesToValidSpv(const std::string& shader, shaderc_shader_kind kind,
                          const CompileOptions& options) const {
    return IsValidSpv(compiler_.CompileGlslToSpv(shader, kind, options));
  }

  // Compiles a shader, asserts compilation success, and returns the warning
  // messages.
  std::string CompilationWarnings(
      const std::string& shader, shaderc_shader_kind kind,
      // This could default to options_, but that can
      // be easily confused with a no-options-provided
      // case:
      const CompileOptions& options) {
    const auto module = compiler_.CompileGlslToSpv(shader, kind, options);
    EXPECT_TRUE(module.GetSuccess()) << kind << '\n' << shader;
    return module.GetErrorMessage();
  }

  // For compiling shaders in subclass tests:
  shaderc::Compiler compiler_;
  CompileOptions options_;

 private:
  static bool IsValidSpv(const shaderc::SpvModule& result) {
    if (!result.GetSuccess()) return false;
    size_t length = result.GetLength();
    if (length < 20) return false;
    const uint32_t* bytes = static_cast<const uint32_t*>(
        static_cast<const void*>(result.GetData()));
    return bytes[0] == spv::MagicNumber;
  }
};

TEST_F(CppInterface, MultipleCalls) {
  shaderc::Compiler compiler1, compiler2, compiler3;
  EXPECT_TRUE(compiler1.IsValid());
  EXPECT_TRUE(compiler2.IsValid());
  EXPECT_TRUE(compiler3.IsValid());
}

TEST_F(CppInterface, MultipleThreadsInitializing) {
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

TEST_F(CppInterface, CompilerMoves) {
  ASSERT_TRUE(compiler_.IsValid());
  shaderc::Compiler compiler2(std::move(compiler_));
  ASSERT_FALSE(compiler_.IsValid());
  ASSERT_TRUE(compiler2.IsValid());
}

TEST_F(CppInterface, EmptyString) {
  ASSERT_TRUE(compiler_.IsValid());
  EXPECT_FALSE(CompilationSuccess("", shaderc_glsl_vertex_shader));
  EXPECT_FALSE(CompilationSuccess("", shaderc_glsl_fragment_shader));
}

TEST_F(CppInterface, ModuleMoves) {
  ASSERT_TRUE(compiler_.IsValid());
  shaderc::SpvModule result =
      compiler_.CompileGlslToSpv(kMinimalShader, shaderc_glsl_vertex_shader);
  EXPECT_TRUE(result.GetSuccess());
  shaderc::SpvModule result2(std::move(result));
  EXPECT_FALSE(result.GetSuccess());
  EXPECT_TRUE(result2.GetSuccess());
}

TEST_F(CppInterface, GarbageString) {
  ASSERT_TRUE(compiler_.IsValid());
  EXPECT_FALSE(CompilationSuccess("jfalkds", shaderc_glsl_vertex_shader));
  EXPECT_FALSE(CompilationSuccess("jfalkds", shaderc_glsl_fragment_shader));
}

TEST_F(CppInterface, MinimalShader) {
  ASSERT_TRUE(compiler_.IsValid());
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_vertex_shader));
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_fragment_shader));
}

TEST_F(CppInterface, BasicOptions) {
  ASSERT_TRUE(compiler_.IsValid());
  EXPECT_TRUE(
      CompilesToValidSpv(kMinimalShader, shaderc_glsl_vertex_shader, options_));
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_fragment_shader,
                                 options_));
}

TEST_F(CppInterface, CopiedOptions) {
  ASSERT_TRUE(compiler_.IsValid());
  EXPECT_TRUE(
      CompilesToValidSpv(kMinimalShader, shaderc_glsl_vertex_shader, options_));
  CompileOptions copied_options(options_);
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_fragment_shader,
                                 copied_options));
}

TEST_F(CppInterface, MovedOptions) {
  ASSERT_TRUE(compiler_.IsValid());
  EXPECT_TRUE(
      CompilesToValidSpv(kMinimalShader, shaderc_glsl_vertex_shader, options_));
  CompileOptions copied_options(std::move(options_));
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_fragment_shader,
                                 copied_options));
}

TEST_F(CppInterface, StdAndCString) {
  ASSERT_TRUE(compiler_.IsValid());
  shaderc::SpvModule result1 = compiler_.CompileGlslToSpv(
      kMinimalShader, strlen(kMinimalShader), shaderc_glsl_fragment_shader);
  shaderc::SpvModule result2 = compiler_.CompileGlslToSpv(
      std::string(kMinimalShader), shaderc_glsl_fragment_shader);
  EXPECT_TRUE(result1.GetSuccess());
  EXPECT_TRUE(result2.GetSuccess());
  EXPECT_EQ(result1.GetLength(), result2.GetLength());
  EXPECT_EQ(std::vector<char>(result1.GetData(),
                              result1.GetData() + result1.GetLength()),
            std::vector<char>(result2.GetData(),
                              result2.GetData() + result2.GetLength()));
}

TEST_F(CppInterface, ErrorsReported) {
  ASSERT_TRUE(compiler_.IsValid());
  shaderc::SpvModule result = compiler_.CompileGlslToSpv(
      "int f(){return wrongname;}", shaderc_glsl_vertex_shader);
  ASSERT_FALSE(result.GetSuccess());
  EXPECT_THAT(result.GetErrorMessage(), HasSubstr("wrongname"));
}

TEST_F(CppInterface, MultipleThreadsCalling) {
  ASSERT_TRUE(compiler_.IsValid());
  bool results[10];
  std::vector<std::thread> threads;
  for (auto& r : results) {
    threads.emplace_back([this, &r]() {
      r = CompilationSuccess(kMinimalShader, shaderc_glsl_vertex_shader);
    });
  }
  for (auto& t : threads) {
    t.join();
  }
  EXPECT_THAT(results, Each(true));
}

TEST_F(CppInterface, AccessorsOnNullModule) {
  shaderc::SpvModule result(nullptr);
  EXPECT_FALSE(result.GetSuccess());
  EXPECT_EQ(std::string(), result.GetErrorMessage());
  EXPECT_EQ(std::string(), result.GetData());
  EXPECT_EQ(0u, result.GetLength());
}

TEST_F(CppInterface, MacroCompileOptions) {
  options_.AddMacroDefinition("E", "main");
  const std::string kMinimalExpandedShader = "void E(){}";
  const std::string kMinimalDoubleExpandedShader = "F E(){}";
  EXPECT_TRUE(CompilationSuccess(kMinimalExpandedShader,
                                 shaderc_glsl_vertex_shader, options_));

  CompileOptions cloned_options(options_);
  // The simplest should still compile with the cloned options.
  EXPECT_TRUE(CompilationSuccess(kMinimalExpandedShader,
                                 shaderc_glsl_vertex_shader, cloned_options));

  EXPECT_FALSE(CompilationSuccess(kMinimalDoubleExpandedShader,
                                  shaderc_glsl_vertex_shader, cloned_options));

  cloned_options.AddMacroDefinition("F", "void");
  // This should still not work with the original options.
  EXPECT_FALSE(CompilationSuccess(kMinimalDoubleExpandedShader,
                                  shaderc_glsl_vertex_shader, options_));
  // This should work with the cloned options that have the additional
  // parameter.
  EXPECT_TRUE(CompilationSuccess(kMinimalDoubleExpandedShader,
                                 shaderc_glsl_vertex_shader, cloned_options));
}

TEST_F(CppInterface, DisassemblyOption) {
  options_.SetDisassemblyMode();
  shaderc::SpvModule result = compiler_.CompileGlslToSpv(
      kMinimalShader, shaderc_glsl_vertex_shader, options_);
  EXPECT_TRUE(result.GetSuccess());
  // This should work with both the glslang native disassembly format and the
  // SPIR-V Tools assembly format.
  EXPECT_THAT(result.GetData(), HasSubstr("Capability Shader"));
  EXPECT_THAT(result.GetData(), HasSubstr("MemoryModel"));

  CompileOptions cloned_options(options_);
  shaderc::SpvModule result_from_cloned_options = compiler_.CompileGlslToSpv(
      kMinimalShader, shaderc_glsl_vertex_shader, cloned_options);
  EXPECT_TRUE(result_from_cloned_options.GetSuccess());
  // The mode should be carried into any clone of the original option object.
  EXPECT_THAT(result_from_cloned_options.GetData(),
              HasSubstr("Capability Shader"));
  EXPECT_THAT(result_from_cloned_options.GetData(), HasSubstr("MemoryModel"));
}

TEST_F(CppInterface, PreprocessingOnlyOption) {
  options_.SetPreprocessingOnlyMode();
  shaderc::SpvModule result = compiler_.CompileGlslToSpv(
      kMinimalShaderWithMacro, shaderc_glsl_vertex_shader, options_);
  EXPECT_TRUE(result.GetSuccess());
  EXPECT_THAT(result.GetData(), HasSubstr("void main(){ }"));

  const std::string kMinimalShaderCloneOption =
      "#define E_CLONE_OPTION main\n"
      "void E_CLONE_OPTION(){}\n";
  CompileOptions cloned_options(options_);
  shaderc::SpvModule result_from_cloned_options = compiler_.CompileGlslToSpv(
      kMinimalShaderCloneOption, shaderc_glsl_vertex_shader, cloned_options);
  EXPECT_TRUE(result_from_cloned_options.GetSuccess());
  EXPECT_THAT(result_from_cloned_options.GetData(),
              HasSubstr("void main(){ }"));
}

TEST_F(CppInterface, PreprocessingOnlyModeFirstOverridesDisassemblyMode) {
  // Sets preprocessing only mode first, then sets disassembly mode.
  // Preprocessing only mode should override disassembly mode.
  options_.SetPreprocessingOnlyMode();
  options_.SetDisassemblyMode();
  shaderc::SpvModule result_preprocessing_mode_first =
      compiler_.CompileGlslToSpv(kMinimalShaderWithMacro,
                                 shaderc_glsl_vertex_shader, options_);
  EXPECT_TRUE(result_preprocessing_mode_first.GetSuccess());
  EXPECT_THAT(result_preprocessing_mode_first.GetData(),
              HasSubstr("void main(){ }"));
}

TEST_F(CppInterface, PreprocessingOnlyModeSecondOverridesDisassemblyMode) {
  // Sets disassembly mode first, then preprocessing only mode.
  // Preprocessing only mode should still override disassembly mode.
  options_.SetDisassemblyMode();
  options_.SetPreprocessingOnlyMode();
  shaderc::SpvModule result_disassembly_mode_first = compiler_.CompileGlslToSpv(
      kMinimalShaderWithMacro, shaderc_glsl_vertex_shader, options_);
  EXPECT_TRUE(result_disassembly_mode_first.GetSuccess());
  EXPECT_THAT(result_disassembly_mode_first.GetData(),
              HasSubstr("void main(){ }"));
}

TEST_F(CppInterface, WarningsOnLine) {
  // By default the compiler will emit a warning on line 2 complaining
  // that 'float' is a deprecated attribute in version 130.
  EXPECT_THAT(
      CompilationWarnings(kDeprecatedAttributeShader,
                          shaderc_glsl_vertex_shader, CompileOptions()),
      HasSubstr(":2: warning: attribute deprecated in version 130; may be "
                "removed in future release\n"));
}

TEST_F(CppInterface, SuppressWarningsOnLine) {
  // Sets the compiler to suppress warnings, so that the deprecated attribute
  // warning won't be emitted.
  options_.SetSuppressWarnings();
  EXPECT_EQ("", CompilationWarnings(kDeprecatedAttributeShader,
                                    shaderc_glsl_vertex_shader, options_));
}

TEST_F(CppInterface, SuppressWarningsOnLineClonedOptions) {
  // Sets the compiler to suppress warnings, so that the deprecated attribute
  // warning won't be emitted, and the mode should be carried into any clone of
  // the original option object.
  options_.SetSuppressWarnings();
  CompileOptions cloned_options(options_);
  EXPECT_EQ("",
            CompilationWarnings(kDeprecatedAttributeShader,
                                shaderc_glsl_vertex_shader, cloned_options));
}

TEST_F(CppInterface, GlobalWarnings) {
  // By default the compiler will emit a warning as version 550 is an unknown
  // version.
  EXPECT_THAT(CompilationWarnings(kMinimalUnknownVersionShader,
                                  shaderc_glsl_vertex_shader, options_),
              HasSubstr("warning: version 550 is unknown.\n"));
}

TEST_F(CppInterface, SuppressGlobalWarnings) {
  // Sets the compiler to suppress warnings, so that the unknown version warning
  // won't be emitted.
  options_.SetSuppressWarnings();
  EXPECT_EQ("", CompilationWarnings(kMinimalUnknownVersionShader,
                                    shaderc_glsl_vertex_shader, options_));
}

TEST_F(CppInterface, SuppressGlobalWarningsClonedOptions) {
  // Sets the compiler to suppress warnings, so that the unknown version warning
  // won't be emitted, and the mode should be carried into any clone of the
  // original option object.
  options_.SetSuppressWarnings();
  CompileOptions cloned_options(options_);
  EXPECT_EQ("",
            CompilationWarnings(kMinimalUnknownVersionShader,
                                shaderc_glsl_vertex_shader, cloned_options));
}

TEST_F(CppInterface, TargetEnvCompileOptions) {
  // Test shader compilation which requires opengl compatibility environment
  options_.SetTargetEnvironment(shaderc_target_env_opengl_compat, 0);
  const std::string kGlslShader =
      R"(#version 100
       uniform highp sampler2D tex;
       void main() {
         gl_FragColor = texture2D(tex, vec2(0.0,0.0));
       }
  )";

  EXPECT_TRUE(
      CompilationSuccess(kGlslShader, shaderc_glsl_fragment_shader, options_));
}

}  // anonymous namespace
