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

#include "shaderc.h"
#include "common_shaders_for_test.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <unordered_map>

#include "SPIRV/spirv.hpp"

namespace {

using testing::Each;
using testing::HasSubstr;

TEST(Init, MultipleCalls) {
  shaderc_compiler_t compiler1, compiler2, compiler3;
  EXPECT_NE(nullptr, compiler1 = shaderc_compiler_initialize());
  EXPECT_NE(nullptr, compiler2 = shaderc_compiler_initialize());
  EXPECT_NE(nullptr, compiler3 = shaderc_compiler_initialize());
  shaderc_compiler_release(compiler1);
  shaderc_compiler_release(compiler2);
  shaderc_compiler_release(compiler3);
}

TEST(Init, MultipleThreadsCalling) {
  shaderc_compiler_t compiler1, compiler2, compiler3;
  std::thread t1([&compiler1]() { compiler1 = shaderc_compiler_initialize(); });
  std::thread t2([&compiler2]() { compiler2 = shaderc_compiler_initialize(); });
  std::thread t3([&compiler3]() { compiler3 = shaderc_compiler_initialize(); });
  t1.join();
  t2.join();
  t3.join();
  EXPECT_NE(nullptr, compiler1);
  EXPECT_NE(nullptr, compiler2);
  EXPECT_NE(nullptr, compiler3);
  shaderc_compiler_release(compiler1);
  shaderc_compiler_release(compiler2);
  shaderc_compiler_release(compiler3);
}

TEST(Init, SPVVersion) {
  unsigned int version = 0;
  unsigned int revision = 0;
  shaderc_get_spv_version(&version, &revision);
  EXPECT_EQ(spv::Version, version);
  EXPECT_EQ(spv::Revision, revision);
}

// RAII class for shaderc_spv_module.
class Compilation {
 public:
  // Compiles shader, keeping the result.
  Compilation(const shaderc_compiler_t compiler, const std::string& shader,
              shaderc_shader_stage shader_stage, const char* input_file_name,
              const shaderc_compile_options_t options = nullptr)
      : compiled_result_(shaderc_compile_into_spv(
            compiler, shader.c_str(), shader.size(), shader_stage,
            input_file_name, "", options)) {}

  ~Compilation() { shaderc_module_release(compiled_result_); }

  shaderc_spv_module_t result() const { return compiled_result_; }

 private:
  shaderc_spv_module_t compiled_result_;
};

struct CleanupOptions {
  void operator()(shaderc_compile_options_t options) const {
    shaderc_compile_options_release(options);
  }
};

typedef std::unique_ptr<shaderc_compile_options, CleanupOptions>
    compile_options_ptr;

// RAII class for shaderc_compiler_t
class Compiler {
 public:
  Compiler() { compiler = shaderc_compiler_initialize(); }
  ~Compiler() { shaderc_compiler_release(compiler); }
  shaderc_compiler_t get_compiler_handle() { return compiler; }

 private:
  shaderc_compiler_t compiler;
};

// Helper function to check if the compilation result indicates a successful
// compilation.
bool CompilationResultIsSuccess(const shaderc_spv_module_t result_module) {
  return shaderc_module_get_compilation_status(result_module) ==
         shaderc_compilation_status_success;
}

// Compiles a shader and returns true if the result is valid SPIR-V.
bool CompilesToValidSpv(Compiler& compiler, const std::string& shader,
                        shaderc_shader_stage shader_stage,
                        const shaderc_compile_options_t options = nullptr) {
  const Compilation comp(compiler.get_compiler_handle(), shader, shader_stage,
                         "shader", options);
  auto result = comp.result();
  if (!CompilationResultIsSuccess(result)) return false;
  size_t length = shaderc_module_get_length(result);
  if (length < 20) return false;
  const uint32_t* bytes = static_cast<const uint32_t*>(
      static_cast<const void*>(shaderc_module_get_bytes(result)));
  return bytes[0] == spv::MagicNumber;
}

// A testing class to test the compilation of a string with or without options.
// This class wraps the initailization of compiler and compiler options and
// groups the result checking methods. Subclass tests can access the compiler
// object and compiler option object to set their properties. Input file names
// are set to "shader".
class CompileStringTest : public testing::Test {
 protected:
  // Compiles a shader and returns true on success, false on failure.
  bool CompilationSuccess(const std::string& shader,
                          shaderc_shader_stage shader_stage,
                          shaderc_compile_options_t options = nullptr) {
    return CompilationResultIsSuccess(
        Compilation(compiler_.get_compiler_handle(), shader, shader_stage,
                    "shader", options)
            .result());
  }

  // Compiles a shader, expects compilation success, and returns the warning
  // messages.
  const std::string CompilationWarnings(
      const std::string& shader, shaderc_shader_stage shader_stage,
      const shaderc_compile_options_t options = nullptr) {
    const Compilation comp(compiler_.get_compiler_handle(), shader,
                           shader_stage, "shader", options);
    EXPECT_TRUE(CompilationResultIsSuccess(comp.result())) << shader_stage
                                                           << '\n'
                                                           << shader;
    return shaderc_module_get_error_message(comp.result());
  };

  // Compiles a shader, expects compilation failure, and returns the warning
  // messages.
  const std::string CompilationErrors(
      const std::string& shader, shaderc_shader_stage shader_stage,
      const shaderc_compile_options_t options = nullptr) {
    const Compilation comp(compiler_.get_compiler_handle(), shader,
                           shader_stage, "shader", options);
    EXPECT_FALSE(CompilationResultIsSuccess(comp.result())) << shader_stage
                                                            << '\n'
                                                            << shader;
    return shaderc_module_get_error_message(comp.result());
  };

  // Compiles a shader, expects compilation success, and returns the output
  // bytes.
  const std::string CompilationOutput(
      const std::string& shader, shaderc_shader_stage shader_stage,
      const shaderc_compile_options_t options = nullptr) {
    const Compilation comp(compiler_.get_compiler_handle(), shader,
                           shader_stage, "shader", options);
    EXPECT_TRUE(CompilationResultIsSuccess(comp.result())) << shader_stage
                                                           << '\n'
                                                           << shader;
    // Use string(const char* s, size_t n) constructor instead of
    // string(const char* s) to make sure the string has complete binary data.
    // string(const char* s) assumes a null-terminated C-string, which will cut
    // the binary data when it sees a '\0' byte.
    return std::string(shaderc_module_get_bytes(comp.result()),
                       shaderc_module_get_length(comp.result()));
  };

  Compiler compiler_;
  compile_options_ptr options_;

 public:
  CompileStringTest() : options_(shaderc_compile_options_initialize()) {}
};

// Name holders so that we have test cases being grouped with only one real
// compilation class.
using CompileStringWithOptionsTest = CompileStringTest;
using CompileStagesTest = CompileStringTest;

TEST_F(CompileStringTest, EmptyString) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_FALSE(CompilationSuccess("", shaderc_glsl_vertex_shader));
  EXPECT_FALSE(CompilationSuccess("", shaderc_glsl_fragment_shader));
}

TEST_F(CompileStringTest, GarbageString) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_FALSE(CompilationSuccess("jfalkds", shaderc_glsl_vertex_shader));
  EXPECT_FALSE(CompilationSuccess("jfalkds", shaderc_glsl_fragment_shader));
}

TEST_F(CompileStringTest, ReallyLongShader) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  std::string minimal_shader = "";
  minimal_shader += "void foo(){}";
  minimal_shader.append(1024 * 1024 * 8, ' ');  // 8MB of spaces.
  minimal_shader += "void main(){}";
  EXPECT_TRUE(CompilesToValidSpv(compiler_, minimal_shader,
                                 shaderc_glsl_vertex_shader));
  EXPECT_TRUE(CompilesToValidSpv(compiler_, minimal_shader,
                                 shaderc_glsl_fragment_shader));
}

TEST_F(CompileStringTest, MinimalShader) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kMinimalShader,
                                 shaderc_glsl_vertex_shader));
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kMinimalShader,
                                 shaderc_glsl_fragment_shader));
}

TEST_F(CompileStringTest, WorksWithCompileOptions) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kMinimalShader,
                                 shaderc_glsl_vertex_shader, options_.get()));
}

TEST_F(CompileStringTest, GetNumErrors) {
  Compilation comp(compiler_.get_compiler_handle(), kTwoErrorsShader,
                   shaderc_glsl_vertex_shader, "shader");
  // Expects compilation failure and two errors.
  EXPECT_FALSE(CompilationResultIsSuccess(comp.result()));
  EXPECT_EQ(2u, shaderc_module_get_num_errors(comp.result()));
  // Expects the number of warnings to be zero.
  EXPECT_EQ(0u, shaderc_module_get_num_warnings(comp.result()));
}

TEST_F(CompileStringTest, GetNumWarnings) {
  Compilation comp(compiler_.get_compiler_handle(), kTwoWarningsShader,
                   shaderc_glsl_vertex_shader, "shader");
  // Expects compilation success with two warnings.
  EXPECT_TRUE(CompilationResultIsSuccess(comp.result()));
  EXPECT_EQ(2u, shaderc_module_get_num_warnings(comp.result()));
  // Expects the number of errors to be zero.
  EXPECT_EQ(0u, shaderc_module_get_num_errors(comp.result()));
}

TEST_F(CompileStringTest, ZeroErrorsZeroWarnings) {
  Compilation comp(compiler_.get_compiler_handle(), kMinimalShader,
                   shaderc_glsl_vertex_shader, "shader");
  // Expects compilation success with zero warnings.
  EXPECT_TRUE(CompilationResultIsSuccess(comp.result()));
  EXPECT_EQ(0u, shaderc_module_get_num_warnings(comp.result()));
  // Expects the number of errors to be zero.
  EXPECT_EQ(0u, shaderc_module_get_num_errors(comp.result()));
}

TEST_F(CompileStringTest, ErrorTypeUnknownShaderStage) {
  // The shader stage can not be determined, the compilation status should
  // indicate the failure is caused by invalid stage.
  Compilation comp(compiler_.get_compiler_handle(), kMinimalShader,
                   shaderc_glsl_infer_from_source, "shader");
  EXPECT_EQ(shaderc_compilation_status_invalid_stage,
            shaderc_module_get_compilation_status(comp.result()));
}
TEST_F(CompileStringTest, ErrorTypeCompilationError) {
  // The shader stage is valid, the result module's error type field should
  // indicate this compilaion fails due to compilation errors.
  Compilation comp(compiler_.get_compiler_handle(), kTwoErrorsShader,
                   shaderc_glsl_vertex_shader, "shader");
  EXPECT_EQ(shaderc_compilation_status_compilation_error,
            shaderc_module_get_compilation_status(comp.result()));
}

TEST_F(CompileStringWithOptionsTest, CloneCompilerOptions) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  compile_options_ptr options_(shaderc_compile_options_initialize());
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kMinimalShader,
                                 shaderc_glsl_vertex_shader, options_.get()));
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options_.get()));
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kMinimalShader,
                                 shaderc_glsl_vertex_shader,
                                 cloned_options.get()));
}

TEST_F(CompileStringWithOptionsTest, MacroCompileOptions) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  shaderc_compile_options_add_macro_definition(options_.get(), "E", "main");
  const std::string kMinimalExpandedShader = "void E(){}";
  const std::string kMinimalDoubleExpandedShader = "F E(){}";
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kMinimalExpandedShader,
                                 shaderc_glsl_vertex_shader, options_.get()));
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options_.get()));
  // The simplest should still compile with the cloned options.
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kMinimalExpandedShader,
                                 shaderc_glsl_vertex_shader,
                                 cloned_options.get()));
  EXPECT_FALSE(CompilesToValidSpv(compiler_, kMinimalDoubleExpandedShader,
                                  shaderc_glsl_vertex_shader,
                                  cloned_options.get()));

  shaderc_compile_options_add_macro_definition(cloned_options.get(), "F",
                                               "void");
  // This should still not work with the original options.
  EXPECT_FALSE(CompilesToValidSpv(compiler_, kMinimalDoubleExpandedShader,
                                  shaderc_glsl_vertex_shader, options_.get()));
  // This should work with the cloned options that have the additional
  // parameter.
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kMinimalDoubleExpandedShader,
                                 shaderc_glsl_vertex_shader,
                                 cloned_options.get()));
}

TEST_F(CompileStringWithOptionsTest, DisassemblyOption) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  shaderc_compile_options_set_disassembly_mode(options_.get());
  // This should work with both the glslang native disassembly format and the
  // SPIR-V tools assembly format.
  const std::string disassembly_text = CompilationOutput(
      kMinimalShader, shaderc_glsl_vertex_shader, options_.get());
  EXPECT_THAT(disassembly_text, HasSubstr("Capability Shader"));
  EXPECT_THAT(disassembly_text, HasSubstr("MemoryModel"));

  // The mode should be carried into any clone of the original option object.
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options_.get()));
  const std::string disassembly_text_cloned_options = CompilationOutput(
      kMinimalShader, shaderc_glsl_vertex_shader, cloned_options.get());
  EXPECT_THAT(disassembly_text_cloned_options, HasSubstr("Capability Shader"));
  EXPECT_THAT(disassembly_text_cloned_options, HasSubstr("MemoryModel"));
}

TEST_F(CompileStringWithOptionsTest, DisassembleMinimalShader) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  shaderc_compile_options_set_disassembly_mode(options_.get());

  const std::string disassembly_text = CompilationOutput(
      kMinimalShader, shaderc_glsl_vertex_shader, options_.get());
  EXPECT_EQ(kMinimalShaderDisassembly, disassembly_text);
}

TEST_F(CompileStringWithOptionsTest, ForcedVersionProfileCorrectStd) {
  // Forces the version and profile to 450core, which fixes the missing
  // #version.
  shaderc_compile_options_set_forced_version_profile(options_.get(), 450,
                                                     shaderc_profile_core);
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kCoreVertShaderWithoutVersion,
                                 shaderc_glsl_vertex_shader, options_.get()));
}

TEST_F(CompileStringWithOptionsTest,
       ForcedVersionProfileCorrectStdClonedOptions) {
  // Forces the version and profile to 450core, which fixes the missing
  // #version.
  shaderc_compile_options_set_forced_version_profile(options_.get(), 450,
                                                     shaderc_profile_core);
  // This mode should be carried to any clone of the original options object.
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options_.get()));
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kCoreVertShaderWithoutVersion,
                                 shaderc_glsl_vertex_shader,
                                 cloned_options.get()));
}

TEST_F(CompileStringWithOptionsTest, ForcedVersionProfileInvalidModule) {
  // Forces the version and profile to 310es, while the source module is invalid
  // for this version of GLSL. Compilation should fail.
  shaderc_compile_options_set_forced_version_profile(options_.get(), 310,
                                                     shaderc_profile_es);
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_THAT(CompilationErrors(kCoreVertShaderWithoutVersion,
                                shaderc_glsl_vertex_shader, options_.get()),
              HasSubstr("error: 'gl_ClipDistance' : undeclared identifier\n"));
}

TEST_F(CompileStringWithOptionsTest, ForcedVersionProfileConflictingStd) {
  // Forces the version and profile to 450core, which is in conflict with the
  // #version in shader.
  shaderc_compile_options_set_forced_version_profile(options_.get(), 450,
                                                     shaderc_profile_core);
  const std::string kVertexShader =
      std::string("#version 310 es\n") + kCoreVertShaderWithoutVersion;
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_THAT(CompilationWarnings(kVertexShader, shaderc_glsl_vertex_shader,
                                  options_.get()),
              HasSubstr("warning: (version, profile) forced to be (450, core), "
                        "while in source code it is (310, es)\n"));
}

TEST_F(CompileStringWithOptionsTest, ForcedVersionProfileUnknownVersionStd) {
  // Forces the version and profile to 4242core, which is an unknown version.
  shaderc_compile_options_set_forced_version_profile(
      options_.get(), 4242 /*unknown version*/, shaderc_profile_core);
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  // Warning message should complain about the unknown version.
  EXPECT_THAT(CompilationWarnings(kMinimalShader, shaderc_glsl_vertex_shader,
                                  options_.get()),
              HasSubstr("warning: version 4242 is unknown.\n"));
}

TEST_F(CompileStringWithOptionsTest, ForcedVersionProfileVersionsBefore150) {
  // Versions before 150 do not allow a profile token, shaderc_profile_none
  // should be passed down as the profile parameter.
  shaderc_compile_options_set_forced_version_profile(options_.get(), 100,
                                                     shaderc_profile_none);
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_TRUE(CompilationSuccess(kMinimalShader, shaderc_glsl_vertex_shader,
                                 options_.get()));
}

TEST_F(CompileStringWithOptionsTest, ForcedVersionProfileRedundantProfileStd) {
  // Forces the version and profile to 100core. But versions before 150 do not
  // allow a profile token, compilation should fail.
  shaderc_compile_options_set_forced_version_profile(options_.get(), 100,
                                                     shaderc_profile_core);
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_THAT(CompilationErrors(kMinimalShader, shaderc_glsl_vertex_shader,
                                options_.get()),
              HasSubstr("error: #version: versions before 150 do not allow a "
                        "profile token\n"));
}

TEST_F(CompileStringWithOptionsTest, GenerateDebugInfoBinary) {
  shaderc_compile_options_set_generate_debug_info(options_.get());
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  // The binary output should contain the name of the vector: debug_info_sample.
  EXPECT_THAT(CompilationOutput(kMinimalDebugInfoShader,
                                shaderc_glsl_vertex_shader, options_.get()),
              HasSubstr("debug_info_sample"));
}

TEST_F(CompileStringWithOptionsTest, GenerateDebugInfoBinaryClonedOptions) {
  shaderc_compile_options_set_generate_debug_info(options_.get());
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options_.get()));
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  // The binary output should contain the name of the vector: debug_info_sample.
  EXPECT_THAT(
      CompilationOutput(kMinimalDebugInfoShader, shaderc_glsl_vertex_shader,
                        cloned_options.get()),
      HasSubstr("debug_info_sample"));
}

TEST_F(CompileStringWithOptionsTest, GenerateDebugInfoDisassembly) {
  shaderc_compile_options_set_generate_debug_info(options_.get());
  // Sets compiler to disassembly mode so that we can compare its output as
  // a string.
  shaderc_compile_options_set_disassembly_mode(options_.get());
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  // The disassembly output should contain the name of the vector:
  // debug_info_sample.
  EXPECT_THAT(CompilationOutput(kMinimalDebugInfoShader,
                                shaderc_glsl_vertex_shader, options_.get()),
              HasSubstr("debug_info_sample"));
}

TEST_F(CompileStringWithOptionsTest, PreprocessingOnlyOption) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  shaderc_compile_options_set_preprocessing_only_mode(options_.get());
  const std::string kMinimalShaderWithMacro =
      "#define E main\n"
      "void E(){}\n";
  const std::string preprocessed_text = CompilationOutput(
      kMinimalShaderWithMacro, shaderc_glsl_vertex_shader, options_.get());
  EXPECT_THAT(preprocessed_text, HasSubstr("void main(){ }"));

  const std::string kMinimalShaderWithMacroCloneOption =
      "#define E_CLONE_OPTION main\n"
      "void E_CLONE_OPTION(){}\n";
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options_.get()));
  const std::string preprocessed_text_cloned_options =
      CompilationOutput(kMinimalShaderWithMacroCloneOption,
                        shaderc_glsl_vertex_shader, options_.get());
  EXPECT_THAT(preprocessed_text_cloned_options, HasSubstr("void main(){ }"));
}

// A shader stage test cases needs: 1) A shader text with or without #pragma
// annotation, 2) shader stage.
struct ShaderStageTestCase {
  const char* shader_;
  shaderc_shader_stage shader_stage_;
};

// Test the shader stage deduction process. If the shader stage is one of the
// forced ones, the compiler will just try to compile the source code in that
// specified shader stage. If the shader stage is
// shaderc_glsl_deduce_from_pragma,
// the compiler will determine the shader stage from #pragma annotation in the
// source code and emit error if none such annotation is found. When the shader
// stage is one of the default ones, the compiler will fall back to use the
// specified shader stage if and only if #pragma annoation is not found.

// Valid shader stage settings should generate valid SPIR-V code.
using ValidShaderStage = testing::TestWithParam<ShaderStageTestCase>;

TEST_P(ValidShaderStage, ValidSpvCode) {
  const ShaderStageTestCase& test_case = GetParam();
  Compiler compiler;
  EXPECT_TRUE(
      CompilesToValidSpv(compiler, test_case.shader_, test_case.shader_stage_));
}

INSTANTIATE_TEST_CASE_P(
    CompileStringTest, ValidShaderStage,
    testing::ValuesIn(std::vector<ShaderStageTestCase>{
        // Valid default shader stage.
        {kEmpty310ESShader, shaderc_glsl_default_vertex_shader},
        {kEmpty310ESShader, shaderc_glsl_default_fragment_shader},
        {kEmpty310ESShader, shaderc_glsl_default_compute_shader},
        {kGeometryOnlyShader, shaderc_glsl_default_geometry_shader},
        {kTessControlOnlyShader, shaderc_glsl_default_tess_control_shader},
        {kTessEvaluationOnlyShader,
         shaderc_glsl_default_tess_evaluation_shader},

        // #pragma annotation overrides default shader stage.
        {kVertexOnlyShaderWithPragma, shaderc_glsl_default_compute_shader},
        {kFragmentOnlyShaderWithPragma, shaderc_glsl_default_vertex_shader},
        {kTessControlOnlyShaderWithPragma,
         shaderc_glsl_default_fragment_shader},
        {kTessEvaluationOnlyShaderWithPragma,
         shaderc_glsl_default_tess_control_shader},
        {kGeometryOnlyShaderWithPragma,
         shaderc_glsl_default_tess_evaluation_shader},
        {kComputeOnlyShaderWithPragma, shaderc_glsl_default_geometry_shader},

        // Specified non-default shader stage overrides #pragma annotation.
        {kVertexOnlyShaderWithInvalidPragma, shaderc_glsl_vertex_shader},
    }));

using InvalidShaderStage = testing::TestWithParam<ShaderStageTestCase>;

// Invalid shader stage settings should generate errors.
TEST_P(InvalidShaderStage, CompilationShouldFail) {
  const ShaderStageTestCase& test_case = GetParam();
  Compiler compiler;
  EXPECT_FALSE(
      CompilesToValidSpv(compiler, test_case.shader_, test_case.shader_stage_));
}

INSTANTIATE_TEST_CASE_P(
    CompileStringTest, InvalidShaderStage,
    testing::ValuesIn(std::vector<ShaderStageTestCase>{
        // Invalid default shader stage.
        {kVertexOnlyShader, shaderc_glsl_default_fragment_shader},
        // Sets to deduce shader stage from #pragma, but #pragma is defined in
        // the source code.
        {kVertexOnlyShader, shaderc_glsl_infer_from_source},
        // Invalid #pragma cause errors, even though default shader stage is set
        // to valid shader stage.
        {kVertexOnlyShaderWithInvalidPragma,
         shaderc_glsl_default_vertex_shader},
    }));

// To test file inclusion, use an unordered_map as a fake file system to store
// fake files to be included. The unordered_map represents a filesystem by
// mapping filename (or path) string to the contents of that file as a string.
using FakeFS = std::unordered_map<std::string, std::string>;

// An includer test case needs: 1) A fake file system which is actually an
// unordered_map, so that we can resolve the content given a string. A valid
// fake file system must have one entry with key:'root' to specify the start
// shader file for compilation. 2) An string that we expect to see in the
// compilation output.
class IncluderTestCase {
 public:
  IncluderTestCase(FakeFS fake_fs, std::string expected_substring)
      : fake_fs_(fake_fs), expected_substring_(expected_substring) {
    assert(fake_fs_.find("root") != fake_fs_.end() &&
           "Valid fake file system needs a 'root' file\n");
  }

  const FakeFS& fake_fs() const { return fake_fs_; }
  const std::string& expected_substring() const { return expected_substring_; }

 private:
  FakeFS fake_fs_;
  std::string expected_substring_;
};

// A mock class that simulate an includer. C API needs two function pointers
// each for get including data and release the data. This class defined two
// static functions, which wrap their matching member functions, to be passed to
// libshaderc C API.
class TestIncluder {
 public:
  TestIncluder(const FakeFS& fake_fs) : fake_fs_(fake_fs), responses_({}){};

  // Get path and content from the fake file system.
  shaderc_includer_response* GetInclude(const char* filename) {
    responses_.emplace_back(shaderc_includer_response{
        filename, strlen(filename), fake_fs_.at(std::string(filename)).c_str(),
        fake_fs_.at(std::string(filename)).size()});
    return &responses_.back();
  }

  // Response data is owned as private property, no need to release explicitly.
  void ReleaseInclude(shaderc_includer_response*) {}

  // Wrapper for the corresponding member function.
  static shaderc_includer_response* GetIncluderResponseWrapper(
      void* user_data, const char* filename) {
    return static_cast<TestIncluder*>(user_data)->GetInclude(filename);
  }

  // Wrapper for the corresponding member function.
  static void ReleaseIncluderResponseWrapper(void* user_data,
                                             shaderc_includer_response* data) {
    return static_cast<TestIncluder*>(user_data)->ReleaseInclude(data);
  }

 private:
  // Includer response data is stored as private property.
  const FakeFS& fake_fs_;
  std::vector<shaderc_includer_response> responses_;
};

using IncluderTests = testing::TestWithParam<IncluderTestCase>;

// Parameterized tests for includer.
TEST_P(IncluderTests, SetIncluderCallbacks) {
  const IncluderTestCase& test_case = GetParam();
  const FakeFS& fs = test_case.fake_fs();
  const std::string& shader = fs.at("root");
  TestIncluder includer(fs);
  Compiler compiler;
  compile_options_ptr options(shaderc_compile_options_initialize());
  shaderc_compile_options_set_preprocessing_only_mode(options.get());
  shaderc_compile_options_set_includer_callbacks(
      options.get(), TestIncluder::GetIncluderResponseWrapper,
      TestIncluder::ReleaseIncluderResponseWrapper, &includer);

  const Compilation comp(compiler.get_compiler_handle(), shader,
                         shaderc_glsl_vertex_shader, "shader", options.get());
  // Checks the existence of the expected string.
  EXPECT_THAT(shaderc_module_get_bytes(comp.result()),
              HasSubstr(test_case.expected_substring()));
}

TEST_P(IncluderTests, SetIncluderCallbacksClonedOptions) {
  const IncluderTestCase& test_case = GetParam();
  const FakeFS& fs = test_case.fake_fs();
  const std::string& shader = fs.at("root");
  TestIncluder includer(fs);
  Compiler compiler;
  compile_options_ptr options(shaderc_compile_options_initialize());
  shaderc_compile_options_set_preprocessing_only_mode(options.get());
  shaderc_compile_options_set_includer_callbacks(
      options.get(), TestIncluder::GetIncluderResponseWrapper,
      TestIncluder::ReleaseIncluderResponseWrapper, &includer);

  // Cloned options should have all the settings.
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options.get()));

  const Compilation comp(compiler.get_compiler_handle(), shader,
                         shaderc_glsl_vertex_shader, "shader",
                         cloned_options.get());
  // Checks the existence of the expected string.
  EXPECT_THAT(shaderc_module_get_bytes(comp.result()),
              HasSubstr(test_case.expected_substring()));
}

INSTANTIATE_TEST_CASE_P(CompileStringTest, IncluderTests,
                        testing::ValuesIn(std::vector<IncluderTestCase>{
                            IncluderTestCase(
                                // Fake file system.
                                {
                                    {"root",
                                     "void foo() {}\n"
                                     "#include \"path/to/file_1\"\n"},
                                    {"path/to/file_1", "content of file_1\n"},
                                },
                                // Expected output.
                                "#line 0 \"path/to/file_1\"\n"
                                " content of file_1\n"
                                "#line 2"),
                            IncluderTestCase(
                                // Fake file system.
                                {{"root",
                                  "void foo() {}\n"
                                  "#include \"path/to/file_1\"\n"},
                                 {"path/to/file_1",
                                  "#include \"path/to/file_2\"\n"
                                  "content of file_1\n"},
                                 {"path/to/file_2", "content of file_2\n"}},
                                // Expected output.
                                "#line 0 \"path/to/file_1\"\n"
                                "#line 0 \"path/to/file_2\"\n"
                                " content of file_2\n"
                                "#line 1 \"path/to/file_1\"\n"
                                " content of file_1\n"
                                "#line 2"),

                        }));

TEST_F(CompileStringWithOptionsTest, WarningsOnLine) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_THAT(
      CompilationWarnings(kDeprecatedAttributeShader,
                          shaderc_glsl_vertex_shader, options_.get()),
      HasSubstr(":2: warning: attribute deprecated in version 130; may be "
                "removed in future release\n"));
}

TEST_F(CompileStringWithOptionsTest, WarningsOnLineAsErrors) {
  shaderc_compile_options_set_warnings_as_errors(options_.get());
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_THAT(
      CompilationErrors(kDeprecatedAttributeShader, shaderc_glsl_vertex_shader,
                        options_.get()),
      HasSubstr(":2: error: attribute deprecated in version 130; may be "
                "removed in future release\n"));
}

TEST_F(CompileStringWithOptionsTest, SuppressWarningsOnLine) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  shaderc_compile_options_set_suppress_warnings(options_.get());
  EXPECT_EQ("",
            CompilationWarnings(kDeprecatedAttributeShader,
                                shaderc_glsl_vertex_shader, options_.get()));
}

TEST_F(CompileStringWithOptionsTest, GlobalWarnings) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_THAT(CompilationWarnings(kMinimalUnknownVersionShader,
                                  shaderc_glsl_vertex_shader, options_.get()),
              HasSubstr("version 550 is unknown.\n"));
}

TEST_F(CompileStringWithOptionsTest, GlobalWarningsAsErrors) {
  shaderc_compile_options_set_warnings_as_errors(options_.get());
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_THAT(CompilationErrors(kMinimalUnknownVersionShader,
                                shaderc_glsl_vertex_shader, options_.get()),
              HasSubstr("error: version 550 is unknown.\n"));
}

TEST_F(CompileStringWithOptionsTest, SuppressGlobalWarnings) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  shaderc_compile_options_set_suppress_warnings(options_.get());
  EXPECT_EQ("",
            CompilationWarnings(kMinimalUnknownVersionShader,
                                shaderc_glsl_vertex_shader, options_.get()));
}

TEST_F(CompileStringWithOptionsTest,
       SuppressWarningsModeFirstOverridesWarningsAsErrorsMode) {
  // Sets suppress-warnings mode first, then sets warnings-as-errors mode.
  // suppress-warnings mode should override warnings-as-errors mode.
  shaderc_compile_options_set_suppress_warnings(options_.get());
  shaderc_compile_options_set_warnings_as_errors(options_.get());
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());

  // Warnings on line should be inhibited.
  EXPECT_EQ("",
            CompilationWarnings(kDeprecatedAttributeShader,
                                shaderc_glsl_vertex_shader, options_.get()));

  // Global warnings should be inhibited.
  EXPECT_EQ("",
            CompilationWarnings(kMinimalUnknownVersionShader,
                                shaderc_glsl_vertex_shader, options_.get()));
}

TEST_F(CompileStringWithOptionsTest,
       SuppressWarningsModeSecondOverridesWarningsAsErrorsMode) {
  // Sets suppress-warnings mode first, then sets warnings-as-errors mode.
  // suppress-warnings mode should override warnings-as-errors mode.
  shaderc_compile_options_set_warnings_as_errors(options_.get());
  shaderc_compile_options_set_suppress_warnings(options_.get());
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());

  // Warnings on line should be inhibited.
  EXPECT_EQ("",
            CompilationWarnings(kDeprecatedAttributeShader,
                                shaderc_glsl_vertex_shader, options_.get()));

  // Global warnings should be inhibited.
  EXPECT_EQ("",
            CompilationWarnings(kMinimalUnknownVersionShader,
                                shaderc_glsl_vertex_shader, options_.get()));
}

TEST_F(CompileStringWithOptionsTest, IfDefCompileOption) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  shaderc_compile_options_add_macro_definition(options_.get(), "E", nullptr);
  const std::string kMinimalExpandedShader =
      "#ifdef E\n"
      "void main(){}\n"
      "#else\n"
      "#error\n"
      "#endif";
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kMinimalExpandedShader,
                                 shaderc_glsl_vertex_shader, options_.get()));
}

TEST_F(CompileStringWithOptionsTest,
       TargetEnvRespectedWhenCompilingOpenGLCompatibilityShaderToBinary) {
  // Confirm that kOpenGLCompatibilityFragmentShader compiles with
  // shaderc_target_env_opengl_compat.  When targeting OpenGL core profile
  // or Vulkan, it should fail to compile.

  EXPECT_FALSE(CompilesToValidSpv(compiler_, kOpenGLCompatibilityFragmentShader,
                                  shaderc_glsl_fragment_shader,
                                  options_.get()));

  shaderc_compile_options_set_target_env(options_.get(),
                                         shaderc_target_env_opengl_compat, 0);
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kOpenGLCompatibilityFragmentShader,
                                 shaderc_glsl_fragment_shader, options_.get()));

  shaderc_compile_options_set_target_env(options_.get(),
                                         shaderc_target_env_opengl, 0);
  EXPECT_FALSE(CompilesToValidSpv(compiler_, kOpenGLCompatibilityFragmentShader,
                                  shaderc_glsl_fragment_shader,
                                  options_.get()));

  shaderc_compile_options_set_target_env(options_.get(),
                                         shaderc_target_env_vulkan, 0);
  EXPECT_FALSE(CompilesToValidSpv(compiler_, kOpenGLCompatibilityFragmentShader,
                                  shaderc_glsl_fragment_shader,
                                  options_.get()));
}

TEST_F(CompileStringWithOptionsTest,
       TargetEnvRespectedWhenCompilingOpenGLCoreShaderToBinary) {
  // Confirm that kOpenGLVertexShader compiles when targeting OpenGL
  // compatibility or core profiles.

  shaderc_compile_options_set_target_env(options_.get(),
                                         shaderc_target_env_opengl_compat, 0);
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kOpenGLVertexShader,
                                 shaderc_glsl_vertex_shader, options_.get()));

  shaderc_compile_options_set_target_env(options_.get(),
                                         shaderc_target_env_opengl, 0);
  EXPECT_TRUE(CompilesToValidSpv(compiler_, kOpenGLVertexShader,
                                 shaderc_glsl_vertex_shader, options_.get()));

  // TODO(dneto): Check what happens when targeting Vulkan.
}

TEST_F(CompileStringWithOptionsTest, TargetEnvIgnoredWhenPreprocessing) {
  shaderc_compile_options_set_preprocessing_only_mode(options_.get());

  EXPECT_TRUE(CompilationSuccess(kOpenGLCompatibilityFragmentShader,
                                 shaderc_glsl_fragment_shader, options_.get()));

  shaderc_compile_options_set_target_env(options_.get(),
                                         shaderc_target_env_opengl_compat, 0);
  EXPECT_TRUE(CompilationSuccess(kOpenGLCompatibilityFragmentShader,
                                 shaderc_glsl_fragment_shader, options_.get()));

  shaderc_compile_options_set_target_env(options_.get(),
                                         shaderc_target_env_opengl, 0);
  EXPECT_TRUE(CompilationSuccess(kOpenGLCompatibilityFragmentShader,
                                 shaderc_glsl_fragment_shader, options_.get()));

  shaderc_compile_options_set_target_env(options_.get(),
                                         shaderc_target_env_vulkan, 0);
  EXPECT_TRUE(CompilationSuccess(kOpenGLCompatibilityFragmentShader,
                                 shaderc_glsl_fragment_shader, options_.get()));
}

TEST_F(CompileStringTest, ShaderStageRespected) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kVertexShader = "void main(){ gl_Position = vec4(0);}";
  EXPECT_TRUE(CompilationSuccess(kVertexShader, shaderc_glsl_vertex_shader));
  EXPECT_FALSE(CompilationSuccess(kVertexShader, shaderc_glsl_fragment_shader));
}

TEST_F(CompileStringTest, ErrorsReported) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_THAT(CompilationErrors("int f(){return wrongname;}",
                                shaderc_glsl_vertex_shader),
              HasSubstr("wrongname"));
}

TEST_F(CompileStringTest, MultipleThreadsCalling) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  bool results[10];
  std::vector<std::thread> threads;
  for (auto& r : results) {
    threads.emplace_back([&r, this]() {
      r = CompilationSuccess("void main(){}", shaderc_glsl_vertex_shader);
    });
  }
  for (auto& t : threads) {
    t.join();
  }
  EXPECT_THAT(results, Each(true));
}

TEST_F(CompileStagesTest, Vertex) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kVertexShader = "void main(){ gl_Position = vec4(0);}";
  EXPECT_TRUE(CompilationSuccess(kVertexShader, shaderc_glsl_vertex_shader));
}

TEST_F(CompileStagesTest, Fragment) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kFragShader = "void main(){ gl_FragColor = vec4(0);}";
  EXPECT_TRUE(CompilationSuccess(kFragShader, shaderc_glsl_fragment_shader));
}

TEST_F(CompileStagesTest, Compute) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kCompShader =
      R"(#version 310 es
       void main() {}
  )";
  EXPECT_TRUE(CompilationSuccess(kCompShader, shaderc_glsl_compute_shader));
}

TEST_F(CompileStagesTest, Geometry) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kGeoShader =

      R"(#version 310 es
       #extension GL_OES_geometry_shader : enable
       layout(points) in;
       layout(points, max_vertices=1) out;
       void main() {
         gl_Position = vec4(1.0);
         EmitVertex();
         EndPrimitive();
       }
  )";
  EXPECT_TRUE(CompilationSuccess(kGeoShader, shaderc_glsl_geometry_shader));
}

TEST_F(CompileStagesTest, TessControl) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kTessControlShader =
      R"(#version 310 es
       #extension GL_OES_tessellation_shader : enable
       layout(vertices=1) out;
       void main() {}
  )";
  EXPECT_TRUE(
      CompilationSuccess(kTessControlShader, shaderc_glsl_tess_control_shader));
}

TEST_F(CompileStagesTest, TessEvaluation) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kTessEvaluationShader =
      R"(#version 310 es
       #extension GL_OES_tessellation_shader : enable
       layout(triangles, equal_spacing, ccw) in;
       void main() {
         gl_Position = vec4(gl_TessCoord, 1.0);
       }
  )";
  EXPECT_TRUE(CompilationSuccess(kTessEvaluationShader,
                                 shaderc_glsl_tess_evaluation_shader));
}

// A test case for ParseVersionProfileTest needs: 1) the input string, 2)
// expected parsing results, including 'succeed' flag, version value, and
// profile enum.
struct ParseVersionProfileTestCase {
  ParseVersionProfileTestCase(
      const std::string& input_string, bool expected_succeed,
      int expected_version = 0,
      shaderc_profile expected_profile = shaderc_profile_none)
      : input_string_(input_string),
        expected_succeed_(expected_succeed),
        expected_version_(expected_version),
        expected_profile_(expected_profile) {}
  std::string input_string_;
  bool expected_succeed_;
  int expected_version_;
  shaderc_profile expected_profile_;
};

// Test for a helper function to parse version and profile from string.
using ParseVersionProfileTest =
    testing::TestWithParam<ParseVersionProfileTestCase>;

TEST_P(ParseVersionProfileTest, FromNullTerminatedString) {
  const ParseVersionProfileTestCase& test_case = GetParam();
  int version;
  shaderc_profile profile;
  bool succeed = shaderc_parse_version_profile(test_case.input_string_.c_str(),
                                               &version, &profile);
  EXPECT_EQ(test_case.expected_succeed_, succeed);
  // check the return version and profile only when the parsing succeeds.
  if (succeed) {
    EXPECT_EQ(test_case.expected_version_, version);
    EXPECT_EQ(test_case.expected_profile_, profile);
  }
}

INSTANTIATE_TEST_CASE_P(
    HelperMethods, ParseVersionProfileTest,
    testing::ValuesIn(std::vector<ParseVersionProfileTestCase>{
        // Valid version profiles
        ParseVersionProfileTestCase("450core", true, 450, shaderc_profile_core),
        ParseVersionProfileTestCase("450compatibility", true, 450,
                                    shaderc_profile_compatibility),
        ParseVersionProfileTestCase("310es", true, 310, shaderc_profile_es),
        ParseVersionProfileTestCase("100", true, 100, shaderc_profile_none),

        // Invalid version profiles, the expected_version and expected_profile
        // doesn't matter as they won't be checked if the tests pass correctly.
        ParseVersionProfileTestCase("totally_wrong", false),
        ParseVersionProfileTestCase("111core", false),
        ParseVersionProfileTestCase("450wrongprofile", false),
        ParseVersionProfileTestCase("", false),
    }));

}  // anonymous namespace
