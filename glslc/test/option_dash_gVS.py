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

import re

import expect
from environment import File, Directory
from glslc_test_framework import inside_glslc_testsuite

MINIMAL_SHADER = '#version 310 es\nvoid main() {}\n'

EMPTY_SHADER_IN_CWD = Directory('.', [File('shader.vert', MINIMAL_SHADER)])

# -gVS emits the NonSemantic import and a DebugSource carrying a second
# operand (the embedded source OpString), which -gV alone does not.
NONSEMANTIC_SOURCE_RE = re.compile(
    r'OpExtInstImport "NonSemantic\.Shader\.DebugInfo\.100"'
    r'[\s\S]*DebugSource %\w+ %\w+')


@inside_glslc_testsuite('OptionGVS')
class TestDashGVSEmitsNonSemanticWithSource(expect.ValidFileContents):
    """Tests that -gVS emits NonSemantic debug info with embedded source."""

    environment = EMPTY_SHADER_IN_CWD
    glslc_args = ['-S', '-gVS', 'shader.vert']
    target_filename = 'shader.vert.spvasm'
    expected_file_contents = NONSEMANTIC_SOURCE_RE
