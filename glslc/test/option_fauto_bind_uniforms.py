# Copyright 2017 The Shaderc Authors. All rights reserved.
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

# A GLSL shader with uniforms without explicit bindings.
GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS = """#version 140
  uniform texture2D my_tex;
  uniform sampler my_sam;
  void main() {
    texture(sampler2D(my_tex,my_sam),vec2(1.0));
  }"""

@inside_glslc_testsuite('OptionFAutoBindUniforms')
class UniformBindingsNotCreatedByDefault(expect.ValidAssemblyFileWithoutSubstr):
    """Tests that the compiler does not generate bindings for uniforms by default."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader]
    # Sufficient to just check one of the uniforms.
    unexpected_assembly_substr = "OpDecorate %my_sam Binding"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FAutoBindingUniformsGeneratesBindings(expect.ValidAssemblyFileWithSubstr):
    """Tests that the compiler generates bindings for uniforms upon request ."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms']
    # Sufficient to just check one of the uniforms.
    expected_assembly_substr = "OpDecorate %my_sam Binding 1"
