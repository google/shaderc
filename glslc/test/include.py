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
from environment import File, Directory


@inside_glslc_testsuite('Include')
class VerifyIncludeOneSibling(expect.StdoutMatch):
    """Tests #including a sibling file."""

    environment = Directory('.', [
        File('a.vert', 'content a\n#include "b"\n'),
        File('b', 'content b\n')])

    glslc_args = ['-E', 'a.vert']

    expected_stdout = 'content a\ncontent b\n'

@inside_glslc_testsuite('Include')
class VerifyCompileIncludeOneSibling(expect.ValidObjectFile):
    """Tests #including a sibling file via full compilation."""

    environment = Directory('.', [
        File('a.vert', 'void foo(){}\n#include "b"\n'),
        File('b', 'void main(){foo();}\n')])

    glslc_args = ['a.vert']

@inside_glslc_testsuite('Include')
class VerifyIncludeWithoutNewline(expect.StdoutMatch):
    """Tests a #include without a newline."""

    environment = Directory('.', [
        File('a.vert', '#include "b"'),
        File('b', 'content b\n')])

    glslc_args = ['-E', 'a.vert']

    expected_stdout = 'content b\n'

@inside_glslc_testsuite('Include')
class VerifyCompileIncludeWithoutNewline(expect.ValidObjectFile):
    """Tests a #include without a newline via full compilation."""

    environment = Directory('.', [
        File('a.vert',
             """void main
             #include "b"
             """),
        File('b',
             """#define PAR ()
             PAR{}
             """)])

    glslc_args = ['a.vert']

@inside_glslc_testsuite('Include')
class VerifyIncludeTwoSiblings(expect.StdoutMatch):
    """Tests #including two sibling files."""

    environment = Directory('.', [
        File('b.vert', '#include "a"\ncontent b\n#include "c"\n'),
        File('a', 'content a\n'),
        File('c', 'content c\n')])

    glslc_args = ['-E', 'b.vert']

    expected_stdout = 'content a\ncontent b\ncontent c\n'

@inside_glslc_testsuite('Include')
class VerifyCompileIncludeTwoSiblings(expect.ValidObjectFile):
    """Tests #including two sibling files via full compilation."""

    environment = Directory('.', [
        File('b.vert',
             """#include "a"
             void bfun(){afun();}
             #include "c"
             """),
        File('a',
             """void afun(){}
             #define BODY {}
             """),
        File('c', 'void main() BODY\n')])

    glslc_args = ['b.vert']

@inside_glslc_testsuite('Include')
class VerifyNestedIncludeAmongSiblings(expect.StdoutMatch):
    """Tests #include inside #included sibling files."""

    environment = Directory('.', [
        File('a.vert', '#include "b"\ncontent a\n'),
        File('b', 'content b\n#include "c"\n'),
        File('c', 'content c\n')])

    glslc_args = ['-E', 'a.vert']

    # TODO(deki): there should be a newline after "content c".  Fix it after we
    # start generating #line in included files.  This seems to only affect -E,
    # though: the actual compilation works as if the newline is there.
    expected_stdout = 'content b\ncontent c content a\n'

@inside_glslc_testsuite('Include')
class VerifyCompileNestedIncludeAmongSiblings(expect.ValidObjectFile):
    """Tests #include inside #included sibling files via full compilation."""

    environment = Directory('.', [
        File('a.vert',
             """#define BODY {}
             #include "b"
             void main(){cfun();}
             """),
        File('b',
             """void bfun() BODY
             #include "c"
             """),
        File('c',
             """#define BF bfun()
             void cfun(){BF;}
             """)])

    glslc_args = ['a.vert']

@inside_glslc_testsuite('Include')
class VerifyIncludeSubdir(expect.StdoutMatch):
    """Tests #including a file from a subdirectory."""

    environment = Directory('.', [
        File('a.vert', 'content a1\n#include "subdir/a"\ncontent a2\n'),
        Directory('subdir', [File('a', 'content suba\n')])])

    glslc_args = ['-E', 'a.vert']

    expected_stdout = 'content a1\ncontent suba\ncontent a2\n'

@inside_glslc_testsuite('Include')
class VerifyCompileIncludeSubdir(expect.ValidObjectFile):
    """Tests #including a file from a subdirectory via full compilation."""

    environment = Directory('.', [
        File('a.vert',
             """#define BODY {}
             #include "subdir/a"
             void afun()BODY
             """),
        Directory('subdir', [File('a', 'void main() BODY\n')])])

    glslc_args = ['a.vert']

@inside_glslc_testsuite('Include')
class VerifyIncludeDeepSubdir(expect.StdoutMatch):
    """Tests #including a file from a subdirectory nested a few levels down."""

    environment = Directory('.', [
        File('a.vert',
             'content a1\n#include "dir/subdir/subsubdir/a"\ncontent a2\n'),
        Directory('dir', [
            Directory('subdir', [
                Directory('subsubdir', [File('a', 'content incl\n')])])])])

    glslc_args = ['-E', 'a.vert']

    expected_stdout = 'content a1\ncontent incl\ncontent a2\n'

@inside_glslc_testsuite('Include')
class VerifyCompileIncludeDeepSubdir(expect.ValidObjectFile):
    """Tests #including a file from a subdirectory nested a few levels down
       via full compilation."""

    environment = Directory('.', [
        File('a.vert',
             """#define BODY {}
             #include "dir/subdir/subsubdir/a"
             void afun()BODY
             """),
        Directory('dir', [
            Directory('subdir', [
                Directory('subsubdir', [File('a', 'void main() BODY\n')])])])])

    glslc_args = ['a.vert']
