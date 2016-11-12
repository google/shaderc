# Copyright 2016 The Shaderc Authors. All rights reserved.
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


def shader_with_tex_offset(offset):
    """Returns a vertex FileShader using a texture access with the given offset."""

    source = """#version 150
              uniform sampler1D tex;
              void main() { vec4 x = textureOffset(tex, 1.0, """ + str(offset) + "); }"
    return FileShader(source, ".vert")


@inside_glslc_testsuite('OptionFLimit')
class TestFLimitNoEqual(expect.ErrorMessage):
    """Tests -flimit without equal."""

    glslc_args = ['-flimit']
    expected_error = ["glslc: error: unknown argument: '-flimit'\n"]


@inside_glslc_testsuite('OptionFLimit')
class TestFLimitJustEqual(expect.ValidObjectFile):
    """Tests -flimit= with no argument."""

    shader = shader_with_tex_offset(0);
    glslc_args = ['-c', shader, '-flimit=']


@inside_glslc_testsuite('OptionFLimit')
class TestFLimitJustEqualMaxOffset(expect.ValidObjectFile):
    """Tests -flimit= with no argument.  The shader uses max offset."""

    shader = shader_with_tex_offset(7);
    glslc_args = ['-c', shader, '-flimit=']


@inside_glslc_testsuite('OptionFLimit')
class TestFLimitJustEqualMinOffset(expect.ValidObjectFile):
    """Tests -flimit= with no argument.  The shader uses min offset."""

    shader = shader_with_tex_offset(-8);
    glslc_args = ['-c', shader, '-flimit=']


@inside_glslc_testsuite('OptionFLimit')
class TestFLimitJustEqualBelowMinOffset(expect.ErrorMessageSubstr):
    """Tests -flimit= with no argument.  The shader uses below min default offset."""

    shader = shader_with_tex_offset(-9);
    glslc_args = ['-c', shader, '-flimit=']
    expected_error_substr = ["'texel offset' : value is out of range"]


@inside_glslc_testsuite('OptionFLimit')
class TestFLimitLowerThanDefaultMinOffset(expect.ValidObjectFile):
    """Tests -flimit= with lower than default argument.  The shader uses below min offset."""

    shader = shader_with_tex_offset(-9);
    glslc_args = ['-c', shader, '-flimit= MinProgramTexelOffset -9 ']
