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

#include "libshaderc_util/compiler.h"

#include <sstream>

#include <gtest/gtest.h>

#include "death_test.h"
#include "libshaderc_util/counting_includer.h"

namespace {

using shaderc_util::Compiler;

// These are the flag combinations Glslang uses to set language
// rules based on the target environment.
const EShMessages kOpenGLCompatibilityRules = EShMsgDefault;
const EShMessages kOpenGLRules = EShMsgSpvRules;
const EShMessages kVulkanRules =
    static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

// A trivial vertex shader
const char kVertexShader[] =
    "#version 100\n"
    "void main() {}";

// A shader that compiles under OpenGL compatibility profile rules,
// but not OpenGL core profile rules.
const char kOpenGLCompatibilityFragShader[] =
    R"(#version 100
       uniform highp sampler2D tex;
       void main() {
         gl_FragColor = texture2D(tex, vec2(0.0,0.0));
       })";

// A shader that compiles under OpenGL compatibility profile rules,
// but not OpenGL core profile rules, even when deducing the stage.
const char kOpenGLCompatibilityFragShaderDeducibleStage[] =
    R"(#version 100
       #pragma shader_stage(fragment)
       uniform highp sampler2D tex;
       void main() {
         gl_FragColor = texture2D(tex, vec2(0.0,0.0));
       })";

// A shader that compiles under OpenGL core profile rules.
const char kOpenGLVertexShader[] =
    R"(#version 150
       void main() { int t = gl_VertexID; })";

// A shader that compiles under OpenGL core profile rules, even when
// deducing the stage.
const char kOpenGLVertexShaderDeducibleStage[] =
    R"(#version 150
       #pragma shader_stage(vertex)
       void main() { int t = gl_VertexID; })";

// A CountingIncluder that never returns valid content for a requested
// file inclusion.
class DummyCountingIncluder : public shaderc_util::CountingIncluder {
 private:
  // Returns a pair of empty strings.
  virtual std::pair<std::string, std::string> include_delegate(
      const char*) const override {
    return std::make_pair(std::string(), std::string());
  }
};

// A test fixture for compiling GLSL shaders.
class CompilerTest : public testing::Test {
 public:
  // Returns true if the given compiler successfully compiles the given shader
  // source for the given shader stage.  No includes are permitted, and shader
  // stage deduction falls back to an invalid shader stage.
  bool SimpleCompilationSucceeds(std::string source, EShLanguage stage) {
    std::function<EShLanguage(std::ostream*, const shaderc_util::string_piece&)>
        stage_callback = [](std::ostream*, const shaderc_util::string_piece&) {
          return EShLangCount;
        };
    std::stringstream out;
    std::stringstream errors;
    size_t total_warnings = 0;
    size_t total_errors = 0;

    const bool result = compiler_.Compile(
        source, stage, "shader", stage_callback, DummyCountingIncluder(), &out,
        &errors, &total_warnings, &total_errors);
    errors_ = errors.str();
    return result;
  }

 protected:
  Compiler compiler_;
  // The error string from the most recent compilation.
  std::string errors_;
};

TEST_F(CompilerTest, SimpleVertexShaderCompilesSuccessfully) {
  EXPECT_TRUE(SimpleCompilationSucceeds(kVertexShader, EShLangVertex));
}

TEST_F(CompilerTest, BadVertexShaderFailsCompilation) {
  EXPECT_FALSE(SimpleCompilationSucceeds(" bogus ", EShLangVertex));
}

TEST_F(CompilerTest, RespectTargetEnvOnOpenGLCompatibilityShader) {
  const EShLanguage stage = EShLangFragment;

  compiler_.SetMessageRules(kOpenGLCompatibilityRules);
  EXPECT_TRUE(SimpleCompilationSucceeds(kOpenGLCompatibilityFragShader, stage));
  compiler_.SetMessageRules(kOpenGLRules);
  EXPECT_FALSE(
      SimpleCompilationSucceeds(kOpenGLCompatibilityFragShader, stage));
  compiler_.SetMessageRules(kVulkanRules);
  EXPECT_FALSE(
      SimpleCompilationSucceeds(kOpenGLCompatibilityFragShader, stage));
  // Default compiler.
  compiler_ = Compiler();
  EXPECT_FALSE(
      SimpleCompilationSucceeds(kOpenGLCompatibilityFragShader, stage));
}

TEST_F(CompilerTest,
       RespectTargetEnvOnOpenGLCompatibilityShaderWhenDeducingStage) {
  const EShLanguage stage = EShLangCount;

  compiler_.SetMessageRules(kOpenGLCompatibilityRules);
  EXPECT_TRUE(SimpleCompilationSucceeds(
      kOpenGLCompatibilityFragShaderDeducibleStage, stage))
      << errors_;
  compiler_.SetMessageRules(kOpenGLRules);
  EXPECT_FALSE(SimpleCompilationSucceeds(
      kOpenGLCompatibilityFragShaderDeducibleStage, stage))
      << errors_;
  compiler_.SetMessageRules(kVulkanRules);
  EXPECT_FALSE(SimpleCompilationSucceeds(
      kOpenGLCompatibilityFragShaderDeducibleStage, stage))
      << errors_;
  // Default compiler.
  compiler_ = Compiler();
  EXPECT_FALSE(SimpleCompilationSucceeds(
      kOpenGLCompatibilityFragShaderDeducibleStage, stage))
      << errors_;
}

TEST_F(CompilerTest, RespectTargetEnvOnOpenGLShader) {
  const EShLanguage stage = EShLangVertex;

  compiler_.SetMessageRules(kOpenGLCompatibilityRules);
  EXPECT_TRUE(SimpleCompilationSucceeds(kOpenGLVertexShader, stage));

  compiler_.SetMessageRules(kOpenGLRules);
  EXPECT_TRUE(SimpleCompilationSucceeds(kOpenGLVertexShader, stage));

  // TODO(dneto): Check Vulkan rules.
}

TEST_F(CompilerTest, RespectTargetEnvOnOpenGLShaderWhenDeducingStage) {
  const EShLanguage stage = EShLangCount;

  compiler_.SetMessageRules(kOpenGLCompatibilityRules);
  EXPECT_TRUE(
      SimpleCompilationSucceeds(kOpenGLVertexShaderDeducibleStage, stage));

  compiler_.SetMessageRules(kOpenGLRules);
  EXPECT_TRUE(
      SimpleCompilationSucceeds(kOpenGLVertexShaderDeducibleStage, stage));

  // TODO(dneto): Check Vulkan rules.
}

TEST_F(CompilerTest, DISABLED_RespectTargetEnvOnVulkanShader) {
  // TODO(dneto): Add test for a shader that should only compile for Vulkan.
}

}  // anonymous namespace
