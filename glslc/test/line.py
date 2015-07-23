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
from environment import Directory, File
from glslc_test_framework import inside_glslc_testsuite


@inside_glslc_testsuite('#line')
class TestPoundVersion310InIncludingFile(
    expect.ReturnCodeIsZero, expect.StdoutMatch, expect.StderrMatch):
    """Tests that #line directives follows the behavior of version 310
    (specifying the line number for the next line) when we find a
    #version 310 directive in the including file."""

    environment = Directory('.', [
        File('a.vert', '#version 310 es\n#include "b.glsl"\n'),
        File('b.glsl', 'void main() {}\n')])
    glslc_args = ['-E', 'a.vert']

    expected_stderr = ''
    expected_stdout = \
"""#version 310 es
#extension GL_GOOGLE_include_directive : enable
#line 1 "a.vert"

#line 1 "b.glsl"
void main(){ }
#line 3 "a.vert"

"""


@inside_glslc_testsuite('#line')
class TestPoundVersion150InIncludingFile(
    expect.ReturnCodeIsZero, expect.StdoutMatch, expect.StderrMatch):
    """Tests that #line directives follows the behavior of version 150
    (specifying the line number for itself) when we find a #version 150
    directive in the including file."""

    environment = Directory('.', [
        File('a.vert', '#version 150\n#include "b.glsl"\n'),
        File('b.glsl', 'void main() {}\n')])
    glslc_args = ['-E', 'a.vert']

    expected_stderr = ''
    expected_stdout = \
"""#version 150
#extension GL_GOOGLE_include_directive : enable
#line 0 "a.vert"

#line 0 "b.glsl"
void main(){ }
#line 2 "a.vert"

"""


@inside_glslc_testsuite('#line')
class TestPoundVersionSyntaxErrorInIncludingFile(expect.ErrorMessage):
    """Tests that error message for #version directive has the correct
    filename and line number."""

    environment = Directory('.', [
        File('a.vert', '#version abc def\n#include "b.glsl"\n'),
        File('b.glsl', 'void main() {}\n')])
    glslc_args = ['-E', 'a.vert']

    expected_error = [
        "a.vert:1: error: '#version' : must occur first in shader\n",
        "a.vert:1: error: '#version' : must be followed by version number\n",
        "a.vert:1: error: '#version' : bad profile name; use es, core, or "
        "compatibility\n",
        "3 errors generated.\n",
    ]


# TODO(antiagainst): now #version in included files results in an error.
# Fix this after #version in included files are supported.
@inside_glslc_testsuite('#line')
class TestPoundVersion310InIncludedFile(expect.ErrorMessage):
    """Tests that #line directives follows the behavior of version 310
    (specifying the line number for the next line) when we find a
    #version 310 directive in the included file."""

    environment = Directory('.', [
        File('a.vert', '#include "b.glsl"\nvoid main() {}'),
        File('b.glsl', '#version 310 es\n')])
    glslc_args = ['-E', 'a.vert']

    expected_error = [
        "a.vert:1: error: '#version' : must occur first in shader\n",
        '1 error generated.\n'
    ]


# TODO(antiagainst): now #version in included files results in an error.
# Fix this after #version in included files are supported.
@inside_glslc_testsuite('#line')
class TestPoundVersion150InIncludedFile(expect.ErrorMessage):
    """Tests that #line directives follows the behavior of version 150
    (specifying the line number for itself) when we find a #version 150
    directive in the included file."""

    environment = Directory('.', [
        File('a.vert', '#include "b.glsl"\nvoid main() {}'),
        File('b.glsl', '#version 150\n')])
    glslc_args = ['-E', 'a.vert']

    expected_error = [
        "a.vert:1: error: '#version' : must occur first in shader\n",
        '1 error generated.\n'
    ]


@inside_glslc_testsuite('#line')
class TestSpaceAroundPoundVersion310InIncludingFile(
    expect.ReturnCodeIsZero, expect.StdoutMatch, expect.StderrMatch):
    """Tests that spaces around #version & #include directive doesn't matter."""

    environment = Directory('.', [
        File('a.vert', '\t #  \t version   310 \t  es\n#\tinclude  "b.glsl"\n'),
        File('b.glsl', 'void main() {}\n')])
    glslc_args = ['-E', 'a.vert']

    expected_stderr = ''
    expected_stdout = \
"""#version 310 es
#extension GL_GOOGLE_include_directive : enable
#line 1 "a.vert"

#line 1 "b.glsl"
void main(){ }
#line 3 "a.vert"

"""


@inside_glslc_testsuite('#line')
class TestSpaceAroundPoundVersion150InIncludingFile(
    expect.ReturnCodeIsZero, expect.StdoutMatch, expect.StderrMatch):
    """Tests that spaces around #version & #include directive doesn't matter."""

    environment = Directory('.', [
        File('a.vert', '  \t #\t\tversion\t   150\t \n# include \t "b.glsl"\n'),
        File('b.glsl', 'void main() {}\n')])
    glslc_args = ['-E', 'a.vert']

    expected_stderr = ''
    expected_stdout = \
"""#version 150
#extension GL_GOOGLE_include_directive : enable
#line 0 "a.vert"

#line 0 "b.glsl"
void main(){ }
#line 2 "a.vert"

"""
