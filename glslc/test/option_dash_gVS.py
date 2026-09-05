# Copyright 2026 The Shaderc Authors. All rights reserved.
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


def debug_info_sample_shader():
    return '#version 140\n void main() { vec4 debug_info_sample; }\n'


@inside_glslc_testsuite('OptionGVS')
class TestDashGVSEmitsNonSemanticWithSource(expect.ValidAssemblyFileWithSubstr):
    """Tests that -gVS emits NonSemantic debug info with embedded source."""

    shader = FileShader(debug_info_sample_shader(), '.vert')
    glslc_args = ['-S', '-gVS', shader]
    expected_assembly_substrings = ['NonSemantic.Shader.DebugInfo.100',
                                    'debug_info_sample']
