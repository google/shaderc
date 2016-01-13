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

#ifndef COMMON_SHADERS_FOR_TESTS_H_
#define COMMON_SHADERS_FOR_TESTS_H_

#ifdef __cplusplus
extern "C" {
#endif

// The minimal shader without #version
const char kMinimalShader[] = "void main(){}";
const char kMinimalShaderWithMacro[] =
    "#define E main\n"
    "void E(){}\n";

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

// Empty 310 es shader. It is valid for vertex, fragment, compute shader kind.
const char kEmpty310ESShader[] =
    "#version 310 es\n"
    "void main() {}\n";

// Vertex only shader.
const char kVertexOnlyShader[] =
    "#version 310 es\n"
    "void main() {\n"
    "    gl_Position = vec4(1.);\n"
    "}";

// TessControl only shader.
const char kTessControlOnlyShader[] =
    "#version 440 core\n"
    "layout(vertices = 3) out;\n"
    "void main() { }";

// TessEvaluation only shader.
const char kTessEvaluationOnlyShader[] =
    "#version 440 core\n"
    "layout(triangles) in;\n"
    "void main() { }";

// Geometry only shader.
const char kGeometryOnlyShader[] =
    "#version 150 core\n"
    "layout (triangles) in;\n"
    "layout (line_strip, max_vertices = 4) out;\n"
    "void main() { }";

// Vertex only shader with #pragma annotation.
const char kVertexOnlyShaderWithPragma[] =
    "#version 310 es\n"
    "#pragma shader_stage(vertex)\n"
    "void main() {\n"
    "    gl_Position = vec4(1.);\n"
    "}";

// Fragment only shader with #pragma annotation.
const char kFragmentOnlyShaderWithPragma[] =
    "#version 310 es\n"
    "#pragma shader_stage(fragment)\n"
    "void main() {\n"
    "    gl_FragDepth = 10.;\n"
    "}";

// TessControl only shader with #pragma annotation.
const char kTessControlOnlyShaderWithPragma[] =
    "#version 440 core\n"
    "#pragma shader_stage(tesscontrol)\n"
    "layout(vertices = 3) out;\n"
    "void main() { }";

// TessEvaluation only shader with #pragma annotation.
const char kTessEvaluationOnlyShaderWithPragma[] =
    "#version 440 core\n"
    "#pragma shader_stage(tesseval)\n"
    "layout(triangles) in;\n"
    "void main() { }";

// Geometry only shader with #pragma annotation.
const char kGeometryOnlyShaderWithPragma[] =
    "#version 150 core\n"
    "#pragma shader_stage(geometry)\n"
    "layout (triangles) in;\n"
    "layout (line_strip, max_vertices = 4) out;\n"
    "void main() { }";

// Compute only shader with #pragma annotation.
const char kComputeOnlyShaderWithPragma[] =
    "#version 310 es\n"
    "#pragma shader_stage(compute)\n"
    "void main() {\n"
    "    uvec3 temp = gl_WorkGroupID;\n"
    "}";

// Vertex only shader with invalid #pragma annotation.
const char kVertexOnlyShaderWithInvalidPragma[] =
    "#version 310 es\n"
    "#pragma shader_stage(fragment)\n"
    "void main() {\n"
    "    gl_Position = vec4(1.);\n"
    "}";

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // COMMON_SHADERS_FOR_TESTS_H_
