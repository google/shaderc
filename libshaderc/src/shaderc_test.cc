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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <unordered_map>

#include "SPIRV/spirv.hpp"

namespace {

using testing::Each;
using testing::HasSubstr;

// The minimal shader without #version
const char kMinimalShader[] = "void main(){}";

// By default the compiler will emit a warning on line 2 complaining
// that 'float' is a deprecated attribute in version 130.
const char kDeprecatedAttributeShader[] =
    "#version 130\n"
    "attribute float x;\n"
    "void main() {}\n";

// By default the compiler will emit a warning as version 550 is an unknown
// version.
const char kMinimalUnknownVersionShader[] =
    "#version 550\n"
    "void main() {}\n";

// gl_ClipDistance doesn't exist in es profile (at least until 3.10).
const char kCoreVertShaderWithoutVersion[] =
    "void main() {\n"
    "gl_ClipDistance[0] = 5.;\n"
    "}\n";

// Generated debug information should contain the name of the vector:
// debug_info_sample.
const char kMinimalDebugInfoShader[] =
    "void main(){\n"
    "vec2 debug_info_sample = vec2(1.0,1.0);\n"
    "}\n";

// Compiler should generate two errors.
const char kTwoErrorsShader[] =
    "#error\n"
    "#error\n"
    "void main(){}\n";

// Compiler should generate two warnings.
const char kTwoWarningsShader[] =
    "#version 130\n"
    "attribute float x;\n"
    "attribute float y;\n"
    "void main(){}\n";

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
              shaderc_shader_kind kind,
              const shaderc_compile_options_t options = nullptr)
      : compiled_result_(shaderc_compile_into_spv(
            compiler, shader.c_str(), shader.size(), kind, "", options)) {}

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

// A testing class to test the compilation of a string with or without options.
// This class wraps the initailization of compiler and compiler options and
// groups the result checking methods. Subclass tests can access the compiler
// object and compiler option object to set their properties.
class CompileStringTest : public testing::Test {
 protected:
  // Compiles a shader and returns true on success, false on failure.
  bool CompilationSuccess(const std::string& shader, shaderc_shader_kind kind,
                          shaderc_compile_options_t options = nullptr) {
    return shaderc_module_get_success(
        Compilation(compiler_.get_compiler_handle(), shader, kind, options)
            .result());
  }

  // Compiles a shader and returns true if the result is valid SPIR-V.
  bool CompilesToValidSpv(const std::string& shader, shaderc_shader_kind kind,
                          const shaderc_compile_options_t options = nullptr) {
    const Compilation comp(compiler_.get_compiler_handle(), shader, kind,
                           options);
    auto result = comp.result();
    if (!shaderc_module_get_success(result)) return false;
    size_t length = shaderc_module_get_length(result);
    if (length < 20) return false;
    const uint32_t* bytes = static_cast<const uint32_t*>(
        static_cast<const void*>(shaderc_module_get_bytes(result)));
    return bytes[0] == spv::MagicNumber;
  }

  // Compiles a shader, expects compilation success, and returns the warning
  // messages.
  const std::string CompilationWarnings(
      const std::string& shader, shaderc_shader_kind kind,
      const shaderc_compile_options_t options = nullptr) {
    const Compilation comp(compiler_.get_compiler_handle(), shader, kind,
                           options);
    EXPECT_TRUE(shaderc_module_get_success(comp.result())) << kind << '\n'
                                                           << shader;
    return shaderc_module_get_error_message(comp.result());
  };

  // Compiles a shader, expects compilation failure, and returns the warning
  // messages.
  const std::string CompilationErrors(
      const std::string& shader, shaderc_shader_kind kind,
      const shaderc_compile_options_t options = nullptr) {
    const Compilation comp(compiler_.get_compiler_handle(), shader, kind,
                           options);
    EXPECT_FALSE(shaderc_module_get_success(comp.result())) << kind << '\n'
                                                            << shader;
    return shaderc_module_get_error_message(comp.result());
  };

  // Compiles a shader, expects compilation success, and returns the output
  // bytes.
  const std::string CompilationOutput(
      const std::string& shader, shaderc_shader_kind kind,
      const shaderc_compile_options_t options = nullptr) {
    const Compilation comp(compiler_.get_compiler_handle(), shader, kind,
                           options);
    EXPECT_TRUE(shaderc_module_get_success(comp.result())) << kind << '\n'
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
using CompileKindsTest = CompileStringTest;

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
  EXPECT_TRUE(CompilesToValidSpv(minimal_shader, shaderc_glsl_vertex_shader));
  EXPECT_TRUE(CompilesToValidSpv(minimal_shader, shaderc_glsl_fragment_shader));
}

TEST_F(CompileStringTest, MinimalShader) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_vertex_shader));
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_fragment_shader));
}

TEST_F(CompileStringTest, WorksWithCompileOptions) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_vertex_shader,
                                 options_.get()));
}

TEST_F(CompileStringWithOptionsTest, CloneCompilerOptions) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  compile_options_ptr options_(shaderc_compile_options_initialize());
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_vertex_shader,
                                 options_.get()));
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options_.get()));
  EXPECT_TRUE(CompilesToValidSpv(kMinimalShader, shaderc_glsl_vertex_shader,
                                 cloned_options.get()));
}

TEST_F(CompileStringWithOptionsTest, MacroCompileOptions) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  shaderc_compile_options_add_macro_definition(options_.get(), "E", "main");
  const std::string kMinimalExpandedShader = "void E(){}";
  const std::string kMinimalDoubleExpandedShader = "F E(){}";
  EXPECT_TRUE(CompilesToValidSpv(kMinimalExpandedShader,
                                 shaderc_glsl_vertex_shader, options_.get()));
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options_.get()));
  // The simplest should still compile with the cloned options.
  EXPECT_TRUE(CompilesToValidSpv(kMinimalExpandedShader,
                                 shaderc_glsl_vertex_shader,
                                 cloned_options.get()));
  EXPECT_FALSE(CompilesToValidSpv(kMinimalDoubleExpandedShader,
                                  shaderc_glsl_vertex_shader,
                                  cloned_options.get()));

  shaderc_compile_options_add_macro_definition(cloned_options.get(), "F",
                                               "void");
  // This should still not work with the original options.
  EXPECT_FALSE(CompilesToValidSpv(kMinimalDoubleExpandedShader,
                                  shaderc_glsl_vertex_shader, options_.get()));
  // This should work with the cloned options that have the additional
  // parameter.
  EXPECT_TRUE(CompilesToValidSpv(kMinimalDoubleExpandedShader,
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

TEST_F(CompileStringWithOptionsTest, ForcedVersionProfileCorrectStd) {
  // Forces the version and profile to 450core, which fixes the missing
  // #version.
  shaderc_compile_options_set_forced_version_profile(options_.get(), 450,
                                                     shaderc_profile_core);
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  EXPECT_TRUE(CompilesToValidSpv(kCoreVertShaderWithoutVersion,
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
  EXPECT_TRUE(CompilesToValidSpv(kCoreVertShaderWithoutVersion,
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

TEST_F(CompileStringWithOptionsTest, GetNumErrors) {
  Compilation comp(compiler_.get_compiler_handle(), kTwoErrorsShader,
                   shaderc_glsl_vertex_shader, options_.get());
  // Expects compilation failure and two errors.
  EXPECT_FALSE(shaderc_module_get_success(comp.result()));
  EXPECT_EQ(2u, shaderc_module_get_num_errors(comp.result()));
  // Expects the number of warnings to be zero.
  EXPECT_EQ(0u, shaderc_module_get_num_warnings(comp.result()));
}

TEST_F(CompileStringWithOptionsTest, GetNumWarnings) {
  Compilation comp(compiler_.get_compiler_handle(), kTwoWarningsShader,
                   shaderc_glsl_vertex_shader, options_.get());
  // Expects compilation success with two warnings.
  EXPECT_TRUE(shaderc_module_get_success(comp.result()));
  EXPECT_EQ(2u, shaderc_module_get_num_warnings(comp.result()));
  // Expects the number of errors to be zero.
  EXPECT_EQ(0u, shaderc_module_get_num_errors(comp.result()));
}

TEST_F(CompileStringWithOptionsTest, ZeroErrorsZeroWarnings) {
  Compilation comp(compiler_.get_compiler_handle(), kMinimalShader,
                   shaderc_glsl_vertex_shader);
  // Expects compilation success with zero warnings.
  EXPECT_TRUE(shaderc_module_get_success(comp.result()));
  EXPECT_EQ(0u, shaderc_module_get_num_warnings(comp.result()));
  // Expects the number of errors to be zero.
  EXPECT_EQ(0u, shaderc_module_get_num_errors(comp.result()));
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

// A mock class that simulate a client of the includer.
class IncluderResponsor {
 public:
  // Create a fake file object, with specified fullpath and content.
  struct FakeFile {
    std::string path;
    std::string content;
  };
  // Use hashmap as a fake file system to store fake files to be included.
  using FakeFS = std::unordered_map<std::string, FakeFile>;
  IncluderResponsor(shaderc_compile_options_t options, FakeFS& fake_fs)
      : options_(options), fake_fs_(fake_fs) {
    shaderc_compile_options_set_includer_callbacks(
        options_, GetIncluderResponseCb, ReleaseIncluderResponseCb, this);
  };

  // Get path and content from the fake file system.
  shaderc_includer_response* GetIncluderResponse(const char* filename) {
    FakeFile& file_to_include = fake_fs_[filename];
    response_.path = file_to_include.path.c_str();
    response_.path_length = file_to_include.path.size();
    response_.content = file_to_include.content.c_str();
    response_.content_length = file_to_include.content.size();
    return &response_;
  };

  // Response data is owned as private property, no need to release explicitly.
  void ReleaseIncluderResponse(shaderc_includer_response* data) { (void)data; };

  // Wrapper for the corresponding member function.
  static shaderc_includer_response* GetIncluderResponseCb(
      void* user_data, const char* filename) {
    auto* responsor_handle = static_cast<IncluderResponsor*>(user_data);
    return responsor_handle->GetIncluderResponse(filename);
  };

  // Wrapper for the corresponding member function.
  static void ReleaseIncluderResponseCb(void* user_data,
                                        shaderc_includer_response* data) {
    auto* responsor_handle = static_cast<IncluderResponsor*>(user_data);
    return responsor_handle->ReleaseIncluderResponse(data);
  };

 private:
  shaderc_compile_options_t options_;
  FakeFS& fake_fs_;
  // Includer response data is stored as private property.
  shaderc_includer_response response_;
};

TEST_F(CompileStringWithOptionsTest, Includer) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kMainIncludingShader =
      "void foo() {}\n"
      "#include \"file_0\"\n"
      "#include \"file_1\"\n";

  IncluderResponsor::FakeFS fake_fs({
      {"file_0", {"path/to/file_0", "void bar() {}\n"}},
      {"file_1", {"path/to/file_1", "void main() {foo(); bar();}\n"}}});

  // Sets callbacks for libshaderc includer.
  IncluderResponsor responsor(options_.get(), fake_fs);
  // Expect the compilation to succeed.
  EXPECT_TRUE(CompilesToValidSpv(kMainIncludingShader,
                                 shaderc_glsl_vertex_shader, options_.get()));

  // Sets compiler to preprocessing only mode, so we can check the result after
  // preprocessing. Expect to see the content of the fake file object in the
  // output of preprocessing.
  shaderc_compile_options_set_preprocessing_only_mode(options_.get());
  std::string preprocessing_output = CompilationOutput(kMainIncludingShader,
                                                      shaderc_glsl_vertex_shader,
                                                      options_.get());
  EXPECT_THAT(preprocessing_output,
              HasSubstr("#line 0 \"path/to/file_0\"\n"
                        " void bar(){ }\n"
                        "#line 2"));
  EXPECT_THAT(preprocessing_output,
              HasSubstr("#line 0 \"path/to/file_1\"\n"
                        " void main(){ foo();bar();}\n"
                        "#line 3"));
}

TEST_F(CompileStringWithOptionsTest, IncluderClonedOptions) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kMainIncludingShader =
      "void foo() {}\n"
      "#include \"file_0\"\n"
      "#include \"file_1\"\n";

  IncluderResponsor::FakeFS fake_fs(
      {{"file_0", {"path/to/file_0", "void bar() {}\n"}},
       {"file_1", {"path/to/file_1", "void main() {foo(); bar();}\n"}}});

  // Sets callbacks for libshaderc includer.
  IncluderResponsor responsor(options_.get(), fake_fs);
  // Clone a new options object.
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options_.get()));
  // Expect the compilation to succeed.
  EXPECT_TRUE(CompilesToValidSpv(
      kMainIncludingShader, shaderc_glsl_vertex_shader, cloned_options.get()));

  // Sets compiler to preprocessing only mode, so we can check the result after
  // preprocessing. Expect to see the content of the fake file object in the
  // output of preprocessing.
  shaderc_compile_options_set_preprocessing_only_mode(cloned_options.get());
  std::string preprocessing_output = CompilationOutput(
      kMainIncludingShader, shaderc_glsl_vertex_shader, cloned_options.get());
  EXPECT_THAT(preprocessing_output, HasSubstr("#line 0 \"path/to/file_0\"\n"
                                              " void bar(){ }\n"
                                              "#line 2"));
  EXPECT_THAT(preprocessing_output, HasSubstr("#line 0 \"path/to/file_1\"\n"
                                              " void main(){ foo();bar();}\n"
                                              "#line 3"));
}

TEST_F(CompileStringWithOptionsTest, IncluderNestedInclude) {
  const std::string kMainIncludingShader =
      "void foo() {}\n"
      "#include \"file_0\"\n";

  IncluderResponsor::FakeFS fake_fs({
      {"file_0",
       {"path/to/file_0",
        "#include \"file_1\"\n"
        "void main() {foo(); bar();}\n"}},
      {"file_1", {"path/to/file_1", "void bar() {}\n"}},
  });

  // Sets callbacks for libshaderc includer.
  IncluderResponsor responsor(options_.get(), fake_fs);
  // Clone a new options object.
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options_.get()));
  EXPECT_TRUE(CompilesToValidSpv(kMainIncludingShader,
                                 shaderc_glsl_vertex_shader, cloned_options.get()));
  // Sets compiler to preprocessing only mode, so we can check the result after
  // preprocessing. Expect to see the content of the fake file object in the
  // output of preprocessing.
  shaderc_compile_options_set_preprocessing_only_mode(cloned_options.get());
  std::string preprocessing_output = CompilationOutput(
      kMainIncludingShader, shaderc_glsl_vertex_shader, cloned_options.get());
  EXPECT_THAT(preprocessing_output, HasSubstr("#line 0 \"path/to/file_0\"\n"
                                              "#line 0 \"path/to/file_1\"\n"
                                              " void bar(){ }\n"
                                              "#line 1 \"path/to/file_0\"\n"
                                              " void main(){ foo();bar();}\n"
                                              "#line 2"));
}

TEST_F(CompileStringWithOptionsTest, IncluderNestedIncludeClonedOptions) {
  const std::string kMainIncludingShader =
      "void foo() {}\n"
      "#include \"file_0\"\n";

  IncluderResponsor::FakeFS fake_fs({
      {"file_0",
       {"path/to/file_0",
        "#include \"file_1\"\n"
        "void main() {foo(); bar();}\n"}},
      {"file_1", {"path/to/file_1", "void bar() {}\n"}},
  });

  // Sets callbacks for libshaderc includer.
  IncluderResponsor responsor(options_.get(), fake_fs);

  EXPECT_TRUE(CompilesToValidSpv(kMainIncludingShader,
                                 shaderc_glsl_vertex_shader, options_.get()));
  // Sets compiler to preprocessing only mode, so we can check the result after
  // preprocessing. Expect to see the content of the fake file object in the
  // output of preprocessing.
  shaderc_compile_options_set_preprocessing_only_mode(options_.get());
  std::string preprocessing_output = CompilationOutput(
      kMainIncludingShader, shaderc_glsl_vertex_shader, options_.get());
  EXPECT_THAT(preprocessing_output, HasSubstr("#line 0 \"path/to/file_0\"\n"
                                              "#line 0 \"path/to/file_1\"\n"
                                              " void bar(){ }\n"
                                              "#line 1 \"path/to/file_0\"\n"
                                              " void main(){ foo();bar();}\n"
                                              "#line 2"));
}

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
  EXPECT_TRUE(CompilesToValidSpv(kMinimalExpandedShader,
                                 shaderc_glsl_vertex_shader, options_.get()));
}

TEST_F(CompileStringWithOptionsTest, TargetEnv) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  // Confirm that this shader compiles with shaderc_target_env_opengl_compat;
  // if targeting Vulkan, glslang will fail to compile it
  const std::string kGlslShader =
      R"(#version 100
       uniform highp sampler2D tex;
       void main() {
         gl_FragColor = texture2D(tex, vec2(0.0,0.0));
       }
  )";

  EXPECT_FALSE(CompilesToValidSpv(kGlslShader, shaderc_glsl_fragment_shader,
                                  options_.get()));

  shaderc_compile_options_set_target_env(options_.get(),
                                         shaderc_target_env_opengl_compat, 0);
  EXPECT_TRUE(CompilesToValidSpv(kGlslShader, shaderc_glsl_fragment_shader,
                                 options_.get()));

  shaderc_compile_options_set_target_env(options_.get(),
                                         shaderc_target_env_vulkan, 0);
  EXPECT_FALSE(CompilesToValidSpv(kGlslShader, shaderc_glsl_fragment_shader,
                                  options_.get()));
}

TEST_F(CompileStringTest, ShaderKindRespected) {
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

TEST_F(CompileKindsTest, Vertex) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kVertexShader = "void main(){ gl_Position = vec4(0);}";
  EXPECT_TRUE(CompilationSuccess(kVertexShader, shaderc_glsl_vertex_shader));
}

TEST_F(CompileKindsTest, Fragment) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kFragShader = "void main(){ gl_FragColor = vec4(0);}";
  EXPECT_TRUE(CompilationSuccess(kFragShader, shaderc_glsl_fragment_shader));
}

TEST_F(CompileKindsTest, Compute) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kCompShader =
      R"(#version 310 es
       void main() {}
  )";
  EXPECT_TRUE(CompilationSuccess(kCompShader, shaderc_glsl_compute_shader));
}

TEST_F(CompileKindsTest, Geometry) {
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

TEST_F(CompileKindsTest, TessControl) {
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

TEST_F(CompileKindsTest, TessEvaluation) {
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

}  // anonymous namespace
