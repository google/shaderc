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

import os.path

import expect
from environment import File, Directory
from glslc_test_framework import inside_glslc_testsuite
from placeholder import FileShader


@inside_glslc_testsuite('WorkDir')
class TestWorkDirNoArg(expect.ErrorMessage):
    """Tests -working-directory. Behavior cribbed from Clang."""

    glslc_args = ['-working-directory']
    expected_error = [
        "glslc: error: argument to '-working-directory' is missing "
        '(expected 1 value)\n',
        'glslc: error: no input files\n']


@inside_glslc_testsuite('WorkDir')
class TestWorkDirEqNoArg(expect.ErrorMessage):
    """Tests -working-directory=<empty>. Behavior cribbed from Clang."""

    glslc_args = ['-working-directory=']
    expected_error = ['glslc: error: no input files\n']


EMPTY_SHADER_IN_SUBDIR = Directory(
    'subdir', [File('shader.vert', 'void main() {}')])

@inside_glslc_testsuite('WorkDir')
class TestWorkDirEqNoArgCompileFile(expect.ValidNamedObjectFile):
    """Tests -working-directory=<empty> when compiling input file."""

    environment = Directory('.', [EMPTY_SHADER_IN_SUBDIR])
    glslc_args = ['-c', '-working-directory=', 'subdir/shader.vert']
    # Output file should be generated into subdir/.
    expected_object_filenames = ('subdir/shader.vert.spv',)


@inside_glslc_testsuite('WorkDir')
class TestWorkDirEqNoArgLinkFile(expect.ValidNamedObjectFile):
    """Tests -working-directory=<empty> when linking input file."""

    environment = Directory('.', [EMPTY_SHADER_IN_SUBDIR])
    glslc_args = ['-working-directory=', 'subdir/shader.vert']
    # Output file should be generated into the current directory, not
    # subdir/, where the source is.
    expected_object_filenames = ('a.spv',)

@inside_glslc_testsuite('WorkDir')
class TestWorkDirCompileFile(expect.ValidNamedObjectFile):
    """Tests -working-directory=<dir> when compiling input file."""

    environment = Directory('.', [EMPTY_SHADER_IN_SUBDIR])
    glslc_args = ['-c', '-working-directory=subdir', 'shader.vert']
    # Output file should be generated into subdir/.
    expected_object_filenames = ('subdir/shader.vert.spv',)


@inside_glslc_testsuite('WorkDir')
class TestWorkDirLinkFile(expect.ValidNamedObjectFile):
    """Tests -working-directory=<dir> when linking input file."""

    environment = Directory('.', [EMPTY_SHADER_IN_SUBDIR])
    glslc_args = ['-working-directory=subdir', 'shader.vert']
    # Output file should be generated into the current directory, not
    # subdir/, where the source is.
    expected_object_filenames = ('a.spv',)


@inside_glslc_testsuite('WorkDir')
class TestWorkDirCompileFileOutput(expect.ValidNamedObjectFile):
    """Tests -working-directory=<dir> when compiling input file and specifying
    output filename."""

    environment = Directory('.', [
        Directory('subdir', [
            Directory('bin', []),
            File('shader.vert', 'void main() {}')
        ])
    ])
    glslc_args = ['-c', '-o', 'bin/spv', '-working-directory=subdir',
                  'shader.vert']
    # Output file should be generated into subdir/bin/.
    expected_object_filenames = ('subdir/bin/spv',)


@inside_glslc_testsuite('WorkDir')
class TestWorkDirLinkFileOutput(expect.ValidNamedObjectFile):
    """Tests -working-directory=<dir> when linking input file and specifying
    output filename."""

    environment = Directory('.', [
        EMPTY_SHADER_IN_SUBDIR,
        Directory('bin', [])
    ])
    glslc_args = ['-o', 'bin/spv', '-working-directory=subdir',
                  'shader.vert']
    # Output file should be generated into bin/, not subdir/bin/.
    expected_object_filenames = ('bin/spv',)


@inside_glslc_testsuite('WorkDir')
class TestWorkDirLinkFileOutputNotExist(expect.ErrorMessage):
    """Tests -working-directory=<dir> when linking input file and specifying
    filename in non-existing directory."""

    environment = Directory('.', [EMPTY_SHADER_IN_SUBDIR])
    glslc_args = ['-o', 'bin/spv', '-working-directory=subdir',
                  'shader.vert']
    # Output file should be generated into bin/, which does not exist.
    expected_error = ['glslc: error: cannot open output file: ',
                      "'bin/spv': No such file or directory\n"]


@inside_glslc_testsuite('WorkDir')
class TestWorkDirArgNoEq(expect.ValidNamedObjectFile):
    """Tests -working-directory <dir>."""

    environment = Directory('.', [EMPTY_SHADER_IN_SUBDIR])
    glslc_args = ['-working-directory', 'subdir', 'shader.vert']
    expected_object_filenames = ('a.spv',)


@inside_glslc_testsuite('WorkDir')
class TestWorkDirEqInArg(expect.ValidNamedObjectFile):
    """Tests -working-directory=<dir-with-equal-sign-inside>."""

    environment = Directory('.', [
        Directory('=subdir', [File('shader.vert', 'void main() {}')]),
    ])
    glslc_args = ['-working-directory==subdir', 'shader.vert']
    expected_object_filenames = ('a.spv',)


@inside_glslc_testsuite('WorkDir')
class TestWorkDirCompileFileAbsolutePath(expect.ValidObjectFile):
    """Tests -working-directory=<dir> when compiling input file with absolute
    path."""

    shader = FileShader('void main() {}', '.vert')
    glslc_args = ['-c', '-working-directory=subdir', shader]


@inside_glslc_testsuite('WorkDir')
class TestWorkDirLinkFileAbsolutePath(expect.ValidNamedObjectFile):
    """Tests -working-directory=<dir> when linking input file with absolute
    path."""

    shader = FileShader('void main() {}', '.vert')
    glslc_args = ['-working-directory=subdir', shader]
    expected_object_filenames = ('a.spv',)
