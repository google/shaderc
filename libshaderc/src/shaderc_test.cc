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

  shaderc_spv_module_t result() { return compiled_result_; }

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

// Compiles a shader and returns true on success, false on failure.
bool CompilationSuccess(const shaderc_compiler_t compiler,
                        const std::string& shader, shaderc_shader_kind kind) {
  return shaderc_module_get_success(
      Compilation(compiler, shader, kind).result());
}

// Compiles a shader and returns true if the result is valid SPIR-V.
bool CompilesToValidSpv(const shaderc_compiler_t compiler,
                        const std::string& shader, shaderc_shader_kind kind,
                        const shaderc_compile_options_t options = nullptr) {
  Compilation comp(compiler, shader, kind, options);
  auto result = comp.result();
  if (!shaderc_module_get_success(result)) return false;
  size_t length = shaderc_module_get_length(result);
  if (length < 20) return false;
  const uint32_t* bytes = static_cast<const uint32_t*>(
      static_cast<const void*>(shaderc_module_get_bytes(result)));
  return bytes[0] == spv::MagicNumber;
}

TEST(CompileString, EmptyString) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  EXPECT_FALSE(CompilationSuccess(compiler.get_compiler_handle(), "",
                                  shaderc_glsl_vertex_shader));
  EXPECT_FALSE(CompilationSuccess(compiler.get_compiler_handle(), "",
                                  shaderc_glsl_fragment_shader));
}

TEST(CompileString, GarbageString) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  EXPECT_FALSE(CompilationSuccess(compiler.get_compiler_handle(), "jfalkds",
                                  shaderc_glsl_vertex_shader));
  EXPECT_FALSE(CompilationSuccess(compiler.get_compiler_handle(), "jfalkds",
                                  shaderc_glsl_fragment_shader));
}

TEST(CompileString, ReallyLongShader) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  std::string minimal_shader = "";
  minimal_shader += "void foo(){}";
  minimal_shader.append(1024 * 1024 * 8, ' ');  // 8MB of spaces.
  minimal_shader += "void main(){}";
  EXPECT_TRUE(CompilesToValidSpv(compiler.get_compiler_handle(), minimal_shader,
                                 shaderc_glsl_vertex_shader));
  EXPECT_TRUE(CompilesToValidSpv(compiler.get_compiler_handle(), minimal_shader,
                                 shaderc_glsl_fragment_shader));
}

TEST(CompileString, MinimalShader) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  const std::string kMinimalShader = "void main(){}";
  EXPECT_TRUE(CompilesToValidSpv(compiler.get_compiler_handle(), kMinimalShader,
                                 shaderc_glsl_vertex_shader));
  EXPECT_TRUE(CompilesToValidSpv(compiler.get_compiler_handle(), kMinimalShader,
                                 shaderc_glsl_fragment_shader));
}

TEST(CompileString, WorksWithCompileOptions) {
  Compiler compiler;
  compile_options_ptr options(shaderc_compile_options_initialize());
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  const std::string kMinimalShader = "void main(){}";
  EXPECT_TRUE(CompilesToValidSpv(compiler.get_compiler_handle(), kMinimalShader,
                                 shaderc_glsl_vertex_shader, options.get()));
}

TEST(CompileStringWithOptions, CloneCompilerOptions) {
  Compiler compiler;
  compile_options_ptr options(shaderc_compile_options_initialize());
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  const std::string kMinimalShader = "void main(){}";
  EXPECT_TRUE(CompilesToValidSpv(compiler.get_compiler_handle(), kMinimalShader,
                                 shaderc_glsl_vertex_shader, options.get()));
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options.get()));
  EXPECT_TRUE(CompilesToValidSpv(compiler.get_compiler_handle(), kMinimalShader,
                                 shaderc_glsl_vertex_shader,
                                 cloned_options.get()));
}

TEST(CompileStringWithOptions, MacroCompileOptions) {
  Compiler compiler;
  compile_options_ptr options(shaderc_compile_options_initialize());
  shaderc_compile_options_add_macro_definition(options.get(), "E", "main");
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  const std::string kMinimalExpandedShader = "void E(){}";
  const std::string kMinimalDoubleExpandedShader = "F E(){}";
  EXPECT_TRUE(CompilesToValidSpv(compiler.get_compiler_handle(),
                                 kMinimalExpandedShader,
                                 shaderc_glsl_vertex_shader, options.get()));
  compile_options_ptr cloned_options(
      shaderc_compile_options_clone(options.get()));
  // The simplest should still compile with the cloned options.
  EXPECT_TRUE(
      CompilesToValidSpv(compiler.get_compiler_handle(), kMinimalExpandedShader,
                         shaderc_glsl_vertex_shader, cloned_options.get()));
  EXPECT_FALSE(CompilesToValidSpv(
      compiler.get_compiler_handle(), kMinimalDoubleExpandedShader,
      shaderc_glsl_vertex_shader, cloned_options.get()));

  shaderc_compile_options_add_macro_definition(cloned_options.get(), "F",
                                               "void");
  // This should still not work with the original options.
  EXPECT_FALSE(CompilesToValidSpv(compiler.get_compiler_handle(),
                                  kMinimalDoubleExpandedShader,
                                  shaderc_glsl_vertex_shader, options.get()));
  // This should work with the cloned options that have the additional
  // parameter.
  EXPECT_TRUE(CompilesToValidSpv(
      compiler.get_compiler_handle(), kMinimalDoubleExpandedShader,
      shaderc_glsl_vertex_shader, cloned_options.get()));
}

TEST(CompileStringWithOptions, IfDefCompileOption) {
  Compiler compiler;
  compile_options_ptr options(shaderc_compile_options_initialize());
  shaderc_compile_options_add_macro_definition(options.get(), "E", nullptr);
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  const std::string kMinimalExpandedShader =
      "#ifdef E\n"
      "void main(){}\n"
      "#else\n"
      "garbage string won't compile\n"
      "#endif";
  EXPECT_TRUE(CompilesToValidSpv(compiler.get_compiler_handle(),
                                 kMinimalExpandedShader,
                                 shaderc_glsl_vertex_shader, options.get()));
}

TEST(CompileString, ShaderKindRespected) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  const std::string kVertexShader = "void main(){ gl_Position = vec4(0);}";
  EXPECT_TRUE(CompilationSuccess(compiler.get_compiler_handle(), kVertexShader,
                                 shaderc_glsl_vertex_shader));
  EXPECT_FALSE(CompilationSuccess(compiler.get_compiler_handle(), kVertexShader,
                                  shaderc_glsl_fragment_shader));
}

TEST(CompileString, ErrorsReported) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  Compilation comp(compiler.get_compiler_handle(), "int f(){return wrongname;}",
                   shaderc_glsl_vertex_shader);
  ASSERT_FALSE(shaderc_module_get_success(comp.result()));
  EXPECT_THAT(shaderc_module_get_error_message(comp.result()),
              HasSubstr("wrongname"));
}

TEST(CompileString, MultipleThreadsCalling) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  bool results[10];
  std::vector<std::thread> threads;
  for (auto& r : results) {
    threads.emplace_back([&compiler, &r]() {
      r = CompilationSuccess(compiler.get_compiler_handle(), "void main(){}",
                             shaderc_glsl_vertex_shader);
    });
  }
  for (auto& t : threads) {
    t.join();
  }
  EXPECT_THAT(results, Each(true));
}

TEST(CompileKinds, Vertex) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  const std::string kVertexShader = "void main(){ gl_Position = vec4(0);}";
  EXPECT_TRUE(CompilationSuccess(compiler.get_compiler_handle(), kVertexShader,
                                 shaderc_glsl_vertex_shader));
}

TEST(CompileKinds, Fragment) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  const std::string kFragShader = "void main(){ gl_FragColor = vec4(0);}";
  EXPECT_TRUE(CompilationSuccess(compiler.get_compiler_handle(), kFragShader,
                                 shaderc_glsl_fragment_shader));
}

TEST(CompileKinds, Compute) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  const std::string kCompShader =
    R"(#version 310 es
       void main() {}
  )";
  EXPECT_TRUE(CompilationSuccess(compiler.get_compiler_handle(), kCompShader,
                                 shaderc_glsl_compute_shader));
}

TEST(CompileKinds, Geometry) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
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
  EXPECT_TRUE(CompilationSuccess(compiler.get_compiler_handle(), kGeoShader,
                                 shaderc_glsl_geometry_shader));
}

TEST(CompileKinds, TessControl) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  const std::string kTCSShader =
    R"(#version 310 es
       #extension GL_OES_tessellation_shader : enable
       layout(vertices=1) out;
       void main() {}
  )";
  EXPECT_TRUE(CompilationSuccess(compiler.get_compiler_handle(), kTCSShader,
                                 shaderc_glsl_tess_control_shader));
}

TEST(CompileKinds, TessEvaluation) {
  Compiler compiler;
  ASSERT_NE(nullptr, compiler.get_compiler_handle());
  const std::string kTESShader =
    R"(#version 310 es
       #extension GL_OES_tessellation_shader : enable
       layout(triangles, equal_spacing, ccw) in;
       void main() {
         gl_Position = vec4(gl_TessCoord, 1.0);
       }
  )";
  EXPECT_TRUE(CompilationSuccess(compiler.get_compiler_handle(), kTESShader,
                                 shaderc_glsl_tess_evaluation_shader));
}

}  // anonymous namespace
