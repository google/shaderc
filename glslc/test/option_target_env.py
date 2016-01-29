# Copyright 2015 The Shaderc Authors. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import expect
from glslc_test_framework import inside_glslc_testsuite
from placeholder import FileShader


def opengl_compat_fragment_shader():
    return """#version 100
uniform highp sampler2D tex;
void main() {
  gl_FragColor = texture2D(tex, vec2(0.0, 0.0));
}"""


def opengl_vertex_shader():
    return """#version 150
void main() { int t = gl_VertexID; }"""


@inside_glslc_testsuite('OptionTargetEnv')
class TestTargetEnvEqOpenglCompatWithOpenGlCompatShader(expect.ValidObjectFile):
    """Tests that compiling OpenGL Compatibility Fragment shader with
    --target-env=opengl_compat works correctly"""
    shader = FileShader(opengl_compat_fragment_shader(), '.frag')
    glslc_args = ['--target-env=opengl_compat', '-c', shader]


@inside_glslc_testsuite('OptionTargetEnv')
class TestTargetEnvEqOpenglWithOpenGlCompatShader(expect.ErrorMessage):
    """Tests the error message of compiling OpenGL Compatibility Fragment shader
    with --target-env=opengl"""
    shader = FileShader(opengl_compat_fragment_shader(), '.frag')
    glslc_args = ['--target-env=opengl', shader]
    expected_error = [shader, ":4: error: 'texture2D' : ",
                      "no matching overloaded function found\n",
                      shader, ":4: error: 'assign' :  ",
                      "cannot convert from 'const float' to 'fragColor ",
                      "mediump 4-component vector of float FragColor'\n",
                      "2 errors generated.\n"]


@inside_glslc_testsuite('OptionTargetEnv')
class TestTargetEnvEqVulkanWithOpenGlCompatShader(expect.ErrorMessage):
    """Tests the error message of compiling OpenGL Compatibility Fragment shader
    with --target-env=vulkan"""
    shader = FileShader(opengl_compat_fragment_shader(), '.frag')
    glslc_args = ['--target-env=vulkan', shader]
    expected_error = [shader, ":4: error: 'texture2D' : ",
                      "no matching overloaded function found\n",
                      shader, ":4: error: 'assign' :  ",
                      "cannot convert from 'const float' to 'fragColor ",
                      "mediump 4-component vector of float FragColor'\n",
                      "2 errors generated.\n"]


@inside_glslc_testsuite('OptionTargetEnv')
class TestTargetEnvEqOpenglCompatWithOpenGlVertexShader(expect.ValidObjectFile):
    """Tests that compiling OpenGL vertex shader with --target-env=opengl_compat
    generates valid SPIR-V code"""
    shader = FileShader(opengl_vertex_shader(), '.vert')
    glslc_args = ['--target-env=opengl_compat', '-c', shader]


@inside_glslc_testsuite('OptionTargetEnv')
class TestTargetEnvEqOpenglWithOpenGlVertexShader(expect.ValidObjectFile):
    """Tests that compiling OpenGL vertex shader with --target-env=opengl
    generates valid SPIR-V code"""
    shader = FileShader(opengl_vertex_shader(), '.vert')
    glslc_args = ['--target-env=opengl', '-c', shader]

@inside_glslc_testsuite('OptionTargetEnv')
class TestTargetEnvEqVulkanWithOpenGlVertexShader(expect.ValidObjectFile):
    """Tests that compiling OpenGL vertex shader with --target-env=vulkan
    generates valid SPIR-V code"""
    shader = FileShader(opengl_vertex_shader(), '.vert')
    glslc_args = ['--target-env=vulkan', '-c', shader]
