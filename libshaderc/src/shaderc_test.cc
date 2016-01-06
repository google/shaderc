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

#include "SPIRV/spirv.hpp"

namespace {

using testing::Each;
using testing::HasSubstr;

// The minimal shader without #version
const std::string kMinimalShader = "void main(){}";

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

  // Compiles a shader, expects compilation fail, and returns the warning
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
    return shaderc_module_get_bytes(comp.result());
  };

  Compiler compiler_;
  compile_options_ptr options_;

 public:
  CompileStringTest() : options_(shaderc_compile_options_initialize()){};
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
      "garbage string won't compile\n"
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
  const std::string kTCSShader =
      R"(#version 310 es
       #extension GL_OES_tessellation_shader : enable
       layout(vertices=1) out;
       void main() {}
  )";
  EXPECT_TRUE(CompilationSuccess(kTCSShader, shaderc_glsl_tess_control_shader));
}

TEST_F(CompileKindsTest, TessEvaluation) {
  ASSERT_NE(nullptr, compiler_.get_compiler_handle());
  const std::string kTESShader =
      R"(#version 310 es
       #extension GL_OES_tessellation_shader : enable
       layout(triangles, equal_spacing, ccw) in;
       void main() {
         gl_Position = vec4(gl_TessCoord, 1.0);
       }
  )";
  EXPECT_TRUE(
      CompilationSuccess(kTESShader, shaderc_glsl_tess_evaluation_shader));
}

}  // anonymous namespace
