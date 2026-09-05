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


def empty_es_310_shader():
    return '#version 310 es\n void main() {}\n'


@inside_glslc_testsuite('OptionGV')
class TestDashGVAcceptedAndEmitsNonSemantic(expect.ValidAssemblyFileWithSubstr):
    """Tests that -gV compiles and emits NonSemantic debug info."""

    shader = FileShader(empty_es_310_shader(), '.vert')
    glslc_args = ['-S', '-gV', shader]
    expected_assembly_substr = 'NonSemantic.Shader.DebugInfo.100'
