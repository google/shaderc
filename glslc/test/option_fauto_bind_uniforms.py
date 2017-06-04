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
GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS = """#version 450
  #extension GL_ARB_sparse_texture2 : enable
  uniform texture2D my_tex;
  uniform sampler my_sam;
  layout(rgba32f) uniform image2D my_img;
  layout(rgba32f) uniform imageBuffer my_imbuf;
  uniform block { float x; float y; } my_ubo;
  buffer B { float z; } my_ssbo;
  void main() {
    texture(sampler2D(my_tex,my_sam),vec2(1.0));
    vec4 t;
    sparseImageLoadARB(my_img,ivec2(0),t);
    imageLoad(my_imbuf,42);
    float x = my_ubo.x;
    my_ssbo.z = 1.1;
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


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FImageBindingBaseOptionRespectedOnImage(expect.ValidAssemblyFileWithSubstr):
    """Tests that -fimage-binding-base value is respected on images."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms', '-fimage-binding-base', '44']
    expected_assembly_substr = "OpDecorate %my_img Binding 44"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FImageBindingBaseOptionRespectedOnImageBuffer(expect.ValidAssemblyFileWithSubstr):
    """Tests that -fimage-binding-base value is respected on image buffers."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms', '-fimage-binding-base', '44']
    expected_assembly_substr = "OpDecorate %my_imbuf Binding 45"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FTextureBindingBaseOptionRespected(expect.ValidAssemblyFileWithSubstr):
    """Tests that -ftexture-binding-base value is respected."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms', '-ftexture-binding-base', '44']
    expected_assembly_substr = "OpDecorate %my_tex Binding 44"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FSamplerBindingBaseOptionRespected(expect.ValidAssemblyFileWithSubstr):
    """Tests that -fsampler-binding-base value is respected."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms', '-fsampler-binding-base', '44']
    expected_assembly_substr = "OpDecorate %my_sam Binding 44"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FUboBindingBaseOptionRespectedOnBuffer(expect.ValidAssemblyFileWithSubstr):
    """Tests that -fubo-binding-base value is respected."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms', '-fubo-binding-base', '44']
    expected_assembly_substr = "OpDecorate %my_ubo Binding 44"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FCbufferBindingBaseOptionRespectedOnBuffer(expect.ValidAssemblyFileWithSubstr):
    """Tests that -fcbuffer-binding-base value is respected."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms', '-fcbuffer-binding-base', '44']
    expected_assembly_substr = "OpDecorate %my_ubo Binding 44"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FImageBindingBaseNeedsValue(expect.ErrorMessageSubstr):
    """Tests that -fimage-binding-base requires a value."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fimage-binding-base']
    expected_error_substr = "error: Option -fimage-binding-base requires at least one argument"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FTextureBindingBaseNeedsValue(expect.ErrorMessageSubstr):
    """Tests that -ftexture-binding-base requires a value."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-ftexture-binding-base']
    expected_error_substr = "error: Option -ftexture-binding-base requires at least one argument"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FSamplerBindingBaseNeedsValue(expect.ErrorMessageSubstr):
    """Tests that -fsampler-binding-base requires a value."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fsampler-binding-base']
    expected_error_substr = "error: Option -fsampler-binding-base requires at least one argument"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FUboBindingBaseNeedsValue(expect.ErrorMessageSubstr):
    """Tests that -fubo-binding-base requires a value."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fubo-binding-base']
    expected_error_substr = "error: Option -fubo-binding-base requires at least one argument"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FImageBindingBaseNeedsNumberValueIfNotStage(expect.ErrorMessageSubstr):
    """Tests that -fimage-binding-base requires a number value."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fimage-binding-base', '9x']
    expected_error_substr = "error: invalid offset value 9x for -fimage-binding-base"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FTextureBindingBaseNeedsNumberValue(expect.ErrorMessageSubstr):
    """Tests that -ftexture-binding-base requires a number value."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-ftexture-binding-base', '9x']
    expected_error_substr = "error: invalid offset value 9x for -ftexture-binding-base"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FSamplerBindingBaseNeedsNumberValue(expect.ErrorMessageSubstr):
    """Tests that -fsampler-binding-base requires a number value."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fsampler-binding-base', '9x']
    expected_error_substr = "error: invalid offset value 9x for -fsampler-binding-base"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FUboBindingBaseNeedsNumberValue(expect.ErrorMessageSubstr):
    """Tests that -fubo-binding-base requires a number value."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fubo-binding-base', '9x']
    expected_error_substr = "error: invalid offset value 9x for -fubo-binding-base"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FImageBindingBaseNeedsUnsignedNumberValue(expect.ErrorMessageSubstr):
    """Tests that -fimage-binding-base requires an unsigned number value."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fimage-binding-base', '-6']
    expected_error_substr = "error: invalid offset value -6 for -fimage-binding-base"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FTextureBindingBaseNeedsUnsignedNumberValue(expect.ErrorMessageSubstr):
    """Tests that -ftexture-binding-base requires an unsigned number value."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-ftexture-binding-base', '-6']
    expected_error_substr = "error: invalid offset value -6 for -ftexture-binding-base"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FSamplerBindingBaseNeedsUnsignedNumberValue(expect.ErrorMessageSubstr):
    """Tests that -fsampler-binding-base requires an unsigned value."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fsampler-binding-base', '-6']
    expected_error_substr = "error: invalid offset value -6 for -fsampler-binding-base"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FUboBindingBaseNeedsUnsignedNumberValue(expect.ErrorMessageSubstr):
    """Tests that -fubo-binding-base requires an unsigned value."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fubo-binding-base', '-6']
    expected_error_substr = "error: invalid offset value -6 for -fubo-binding-base"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FImageBindingBaseForVertOptionRespectedOnImageCompileAsVert(expect.ValidAssemblyFileWithSubstr):
    """Tests that -fimage-binding-base with vert stage value is respected on images."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms', '-fimage-binding-base', 'vert', '44']
    expected_assembly_substr = "OpDecorate %my_img Binding 44"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FImageBindingBaseForVertOptionIgnoredOnImageCompileAsFrag(expect.ValidAssemblyFileWithSubstr):
    """Tests that -fimage-binding-base with vert stage value is ignored when cmopiled as
    fragment."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.frag')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms', '-fimage-binding-base', 'vert', '44']
    expected_assembly_substr = "OpDecorate %my_img Binding 2"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FImageBindingBaseForFragOptionRespectedOnImageCompileAsFrag(expect.ValidAssemblyFileWithSubstr):
    """Tests that -fimage-binding-base with frag stage value is respected on images."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.frag')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms', '-fimage-binding-base', 'frag', '44']
    expected_assembly_substr = "OpDecorate %my_img Binding 44"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FSsboBindingBaseRespectedOnSsboCompileAsFrag(expect.ValidAssemblyFileWithSubstr):
    """Tests that -fssbo-binding-base with frag stage value is respected on SSBOs."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.frag')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms', '-fssbo-binding-base', '100']
    expected_assembly_substr = "OpDecorate %my_ssbo Binding 100"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FSsboBindingBaseForFragOptionRespectedOnSsboCompileAsFrag(expect.ValidAssemblyFileWithSubstr):
    """Tests that -fssbo-binding-base with frag stage value is respected on SSBOs."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.frag')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms', '-fssbo-binding-base', 'frag', '100']
    expected_assembly_substr = "OpDecorate %my_ssbo Binding 100"


@inside_glslc_testsuite('OptionFAutoBindUniforms')
class FSsboBindingBaseForFragOptionIgnoredOnSsboCompileAsVert(expect.ValidAssemblyFileWithSubstr):
    """Tests that -fssbo-binding-base with frag stage value is ignored on SSBOs
    when compiling as a vertex shader."""

    shader = FileShader(GLSL_SHADER_WITH_UNIFORMS_WITHOUT_BINDINGS, '.vert')
    glslc_args = ['-S', shader, '-fauto-bind-uniforms', '-fssbo-binding-base', 'frag', '100']
    expected_assembly_substr = "OpDecorate %my_ssbo Binding 5"
