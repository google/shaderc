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

// gl_ClipDistance doesn't exist in es profile (at least until 3.10).
const std::string kCoreVertShaderWithoutVersion =
    "void main() {\n"
    "gl_ClipDistance[0] = 5.;\n"
    "}\n";

// Debug information should contain the name of the vector: debug_info_sample.
const std::string kMinimalDebugInfoShader =
    "void main(){\n"
    "vec2 debug_info_sample = vec2(1.0, 1.0);\n"
    "}\n";

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

  // Compiles a shader, asserts compilation fail, and returns the error
  // messages.
  std::string CompilationErrors(
      const std::string& shader, shaderc_shader_kind kind,
      // This could default to options_, but that can
      // be easily confused with a no-options-provided
      // case:
      const CompileOptions& options) {
    const auto module = compiler_.CompileGlslToSpv(shader, kind, options);
    EXPECT_FALSE(module.GetSuccess()) << kind << '\n' << shader;
    return module.GetErrorMessage();
  }

  // Compiles a shader, expects compilation success, and returns the output
  // bytes.
  std::string CompilationOutput(const std::string& shader,
                                shaderc_shader_kind kind,
                                const CompileOptions& options) const {
    const auto module = compiler_.CompileGlslToSpv(shader, kind, options);
    EXPECT_TRUE(module.GetSuccess()) << kind << '\n';
    // Use string(const char* s, size_t n) constructor instead of
    // string(const char* s) to make sure the string has complete binary data.
    // string(const char* s) assumes a null-terminated C-string, which will cut
    // the binary data when it sees a '\0' byte.
    return std::string(module.GetData(), module.GetLength());
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

TEST_F(CppInterface, CompilerValidUponConstruction) {
  EXPECT_TRUE(compiler_.IsValid());
}

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
  shaderc::Compiler compiler2(std::move(compiler_));
  ASSERT_FALSE(compiler_.IsValid());
  ASSERT_TRUE(compiler2.IsValid());
}

TEST_F(CppInterface, EmptyString) {
  EXPECT_FALSE(CompilationSuccess("", shaderc_glsl_vertex_shader));
  EXPECT_FALSE(CompilationSuccess("", shaderc_glsl_fragment_shader));
}

TEST_F(CppInterface, ModuleMoves) {
  shaderc::SpvModule result =
      compiler_.CompileGlslToSpv(kMinimalShader, shaderc_glsl_vertex_shader);
  EXPECT_TRUE(result.GetSuccess());
  shaderc::SpvModule result2(std::move(result));
  EXPECT_FALSE(result.GetSuccess());
  EXPECT_TRUE(result2.GetSuccess());
}

TEST_F(CppInterface, GarbageString) {
  EXPECT_FALSE(CompilationSuccess("jfalkds", shaderc_glsl_vertex_shader));
  EXPECT_FALSE(CompilationSuccess("jfalkds", shaderc_glsl_fragment_shader));
}

TEST_F(CppInterface, MinimalShader) {
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_vertex_shader));
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_fragment_shader));
}

TEST_F(CppInterface, BasicOptions) {
  EXPECT_TRUE(
      CompilesToValidSpv(kMinimalShader, shaderc_glsl_vertex_shader, options_));
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_fragment_shader,
                                 options_));
}

TEST_F(CppInterface, CopiedOptions) {
  EXPECT_TRUE(
      CompilesToValidSpv(kMinimalShader, shaderc_glsl_vertex_shader, options_));
  CompileOptions copied_options(options_);
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_fragment_shader,
                                 copied_options));
}

TEST_F(CppInterface, MovedOptions) {
  EXPECT_TRUE(
      CompilesToValidSpv(kMinimalShader, shaderc_glsl_vertex_shader, options_));
  CompileOptions copied_options(std::move(options_));
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_fragment_shader,
                                 copied_options));
}

TEST_F(CppInterface, StdAndCString) {
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
  shaderc::SpvModule result = compiler_.CompileGlslToSpv(
      "int f(){return wrongname;}", shaderc_glsl_vertex_shader);
  ASSERT_FALSE(result.GetSuccess());
  EXPECT_THAT(result.GetErrorMessage(), HasSubstr("wrongname"));
}

TEST_F(CppInterface, MultipleThreadsCalling) {
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

TEST_F(CppInterface, ForcedVersionProfileCorrectStd) {
  // Forces the version and profile to 450core, which fixes the missing
  // #version.
  options_.SetForcedVersionProfile(450, shaderc_profile_core);
  EXPECT_TRUE(CompilesToValidSpv(kCoreVertShaderWithoutVersion,
                                 shaderc_glsl_vertex_shader, options_));
}

TEST_F(CppInterface, ForcedVersionProfileCorrectStdClonedOptions) {
  // Forces the version and profile to 450core, which fixes the missing
  // #version.
  options_.SetForcedVersionProfile(450, shaderc_profile_core);
  CompileOptions cloned_options(options_);
  EXPECT_TRUE(CompilesToValidSpv(kCoreVertShaderWithoutVersion,
                                 shaderc_glsl_vertex_shader, cloned_options));
}

TEST_F(CppInterface, ForcedVersionProfileInvalidModule) {
  // Forces the version and profile to 310es, while the source module is invalid
  // for this version of GLSL. Compilation should fail.
  options_.SetForcedVersionProfile(310, shaderc_profile_es);
  EXPECT_THAT(CompilationErrors(kCoreVertShaderWithoutVersion,
                                shaderc_glsl_vertex_shader, options_),
              HasSubstr("error: 'gl_ClipDistance' : undeclared identifier\n"));
}

TEST_F(CppInterface, ForcedVersionProfileConflictingStd) {
  // Forces the version and profile to 450core, which is in conflict with the
  // #version in shader.
  const std::string kVertexShader =
      std::string("#version 310 es\n") + kCoreVertShaderWithoutVersion;
  options_.SetForcedVersionProfile(450, shaderc_profile_core);
  EXPECT_THAT(
      CompilationWarnings(kVertexShader, shaderc_glsl_vertex_shader, options_),
      HasSubstr("warning: (version, profile) forced to be (450, core), "
                "while in source code it is (310, es)\n"));
}

TEST_F(CppInterface, ForcedVersionProfileUnknownVersionStd) {
  // Forces the version and profile to 4242core, which is an unknown version.
  options_.SetForcedVersionProfile(4242 /*unknown version*/,
                                   shaderc_profile_core);
  EXPECT_THAT(
      CompilationWarnings(kMinimalShader, shaderc_glsl_vertex_shader, options_),
      HasSubstr("warning: version 4242 is unknown.\n"));
}

TEST_F(CppInterface, ForcedVersionProfileVersionsBefore150) {
  // Versions before 150 do not allow a profile token, shaderc_profile_none
  // should be passed down as the profile parameter.
  options_.SetForcedVersionProfile(100, shaderc_profile_none);
  EXPECT_TRUE(
      CompilationSuccess(kMinimalShader, shaderc_glsl_vertex_shader, options_));
}

TEST_F(CppInterface, ForcedVersionProfileRedundantProfileStd) {
  // Forces the version and profile to 100core. But versions before 150 do not
  // allow a profile token, compilation should fail.
  options_.SetForcedVersionProfile(100, shaderc_profile_core);
  EXPECT_THAT(
      CompilationErrors(kMinimalShader, shaderc_glsl_vertex_shader, options_),
      HasSubstr("error: #version: versions before 150 do not allow a profile "
                "token\n"));
}

TEST_F(CppInterface, GenerateDebugInfoBinary) {
  options_.SetGenerateDebugInfo();
  // The output binary should contain the name of the vector: debug_info_sample as char
  // array.
  EXPECT_THAT(CompilationOutput(kMinimalDebugInfoShader,
                                shaderc_glsl_vertex_shader, options_),
              HasSubstr("debug_info_sample"));
}

TEST_F(CppInterface, GenerateDebugInfoBinaryClonedOptions) {
  options_.SetGenerateDebugInfo();
  CompileOptions cloned_options(options_);
  // The output binary should contain the name of the vector: debug_info_sample as char
  // array.
  EXPECT_THAT(CompilationOutput(kMinimalDebugInfoShader,
                                shaderc_glsl_vertex_shader, cloned_options),
              HasSubstr("debug_info_sample"));
}

TEST_F(CppInterface, GenerateDebugInfoDisassembly) {
  options_.SetGenerateDebugInfo();
  // Debug info should also be emitted in disassembly mode.
  options_.SetDisassemblyMode();
  // The output disassembly should contain the name of the vector: debug_info_sample.
  EXPECT_THAT(CompilationOutput(kMinimalDebugInfoShader,
                                shaderc_glsl_vertex_shader, options_),
              HasSubstr("debug_info_sample"));
}

TEST_F(CppInterface, GenerateDebugInfoDisassemblyClonedOptions) {
  options_.SetGenerateDebugInfo();
  // Generate debug info mode should be carried to the cloned options.
  CompileOptions cloned_options(options_);
  EXPECT_THAT(CompilationOutput(kMinimalDebugInfoShader,
                                shaderc_glsl_vertex_shader, cloned_options),
              HasSubstr("debug_info_sample"));
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

TEST_F(CppInterface, WarningsOnLineAsErrors) {
  // Sets the compiler to make warnings into errors. So that the deprecated
  // attribute warning will be emitted as an error and compilation should fail.
  options_.SetWarningsAsErrors();
  EXPECT_THAT(
      CompilationErrors(kDeprecatedAttributeShader, shaderc_glsl_vertex_shader,
                        options_),
      HasSubstr(":2: error: attribute deprecated in version 130; may be "
                "removed in future release\n"));
}

TEST_F(CppInterface, WarningsOnLineAsErrorsClonedOptions) {
  // Sets the compiler to make warnings into errors. So that the deprecated
  // attribute warning will be emitted as an error and compilation should fail.
  options_.SetWarningsAsErrors();
  CompileOptions cloned_options(options_);
  // The error message should show an error instead of a warning.
  EXPECT_THAT(
      CompilationErrors(kDeprecatedAttributeShader, shaderc_glsl_vertex_shader,
                        cloned_options),
      HasSubstr(":2: error: attribute deprecated in version 130; may be "
                "removed in future release\n"));
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

TEST_F(CppInterface, GlobalWarningsAsErrors) {
  // Sets the compiler to make warnings into errors. So that the unknown
  // version warning will be emitted as an error and compilation should fail.
  options_.SetWarningsAsErrors();
  EXPECT_THAT(CompilationErrors(kMinimalUnknownVersionShader,
                                shaderc_glsl_vertex_shader, options_),
              HasSubstr("error: version 550 is unknown.\n"));
}

TEST_F(CppInterface, GlobalWarningsAsErrorsClonedOptions) {
  // Sets the compiler to make warnings into errors. This mode should be carried
  // into any clone of the original option object.
  options_.SetWarningsAsErrors();
  CompileOptions cloned_options(options_);
  EXPECT_THAT(CompilationErrors(kMinimalUnknownVersionShader,
                                shaderc_glsl_vertex_shader, cloned_options),
              HasSubstr("error: version 550 is unknown.\n"));
}

TEST_F(CppInterface, SuppressWarningsModeFirstOverridesWarningsAsErrorsMode) {
  // Sets suppress-warnings mode first, then sets warnings-as-errors mode.
  // suppress-warnings mode should override warnings-as-errors mode, no
  // error message should be output for this case.
  options_.SetSuppressWarnings();
  options_.SetWarningsAsErrors();
  // Warnings on line should be inhibited.
  EXPECT_EQ("", CompilationWarnings(kDeprecatedAttributeShader,
                                    shaderc_glsl_vertex_shader, options_));

  // Global warnings should be inhibited.
  EXPECT_EQ("", CompilationWarnings(kMinimalUnknownVersionShader,
                                    shaderc_glsl_vertex_shader, options_));
}

TEST_F(CppInterface, SuppressWarningsModeSecondOverridesWarningsAsErrorsMode) {
  // Sets warnings-as-errors mode first, then sets suppress-warnings mode.
  // suppress-warnings mode should override warnings-as-errors mode, no
  // error message should be output for this case.
  options_.SetWarningsAsErrors();
  options_.SetSuppressWarnings();
  // Warnings on line should be inhibited.
  EXPECT_EQ("", CompilationWarnings(kDeprecatedAttributeShader,
                                    shaderc_glsl_vertex_shader, options_));

  // Global warnings should be inhibited.
  EXPECT_EQ("", CompilationWarnings(kMinimalUnknownVersionShader,
                                    shaderc_glsl_vertex_shader, options_));
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
