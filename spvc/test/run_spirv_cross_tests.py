# Copyright 2019 The Shaderc Authors. All rights reserved.
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

'''
Run the spirv-cross tests on spvc.

Repeat runs are fast because we use a saved, compiled version of each shader
instead of re-compiling.  To re-compile the shaders just delete the cache.
'''

from __future__ import print_function

import errno
import filecmp
import os
import subprocess
import sys
import tempfile

test_case_dirs = (
# directory           language  optimize
('shaders'            , 'glsl', ''   ),
('shaders'            , 'glsl', 'opt'),
('shaders-no-opt'     , 'glsl', ''   ),
('shaders-msl'        , 'msl' , ''   ),
('shaders-msl'        , 'msl' , 'opt'),
('shaders-msl-no-opt' , 'msl' , ''   ),
('shaders-hlsl'       , 'hlsl', ''   ),
('shaders-hlsl'       , 'hlsl', 'opt'),
('shaders-hlsl-no-opt', 'hlsl', ''   ),
('shaders-reflection' , 'refl', ''   ),
)

test_count = 0
pass_count = 0

not_used, tmpfile = tempfile.mkstemp()
devnull = open(os.devnull, 'w')

def mkdir_p(filepath):
    ''' make sure directory exists to contain given file '''
    dirpath = os.path.dirname(filepath)
    try:
        os.makedirs(dirpath)
    except OSError as exc:
        if not (exc.errno == errno.EEXIST and os.path.isdir(dirpath)):
            raise

def spirv_as(inp, out):
    subprocess.check_call([spirv_as_path, '-o', out, inp], stdout=devnull)

def spirv_opt(inp, out):
    subprocess.check_call([spirv_opt_path, '-o', out, inp], stdout=devnull)

def glslangValidator(inp, out):
    subprocess.check_call([glslangValidator_path , '--target-env', 'vulkan1.1', '-V', '-o',
                          out, inp], stdout=devnull)

def spvc(inp, out, flags):
    return subprocess.call([spvc_path, '-o', out, '--validate=vulkan1.1'] + flags + [inp],
                           stdout=devnull)

def compile_shader(source_path, binary_path, filename, optimize):
    # Note: filename is the last component of source_path.
    mkdir_p(binary_path)
    if '.asm.' in filename:
        spirv_as(source_path, binary_path)
    else:
        glslangValidator(source_path, binary_path)
    if optimize:
        spirv_opt(binary_path, binary_path)

def do_test(binary_path, reference_path, flags):
    global test_count, pass_count
    test_count += 1
    if spvc(binary_path, tmpfile, flags) == 0 and filecmp.cmp(tmpfile, reference_path):
        pass_count += 1

def skip_test():
    global test_count
    test_count += 1

def test_shader(binary_path, reference_path, filename, language, optimize):
    # Note: filename is the last component of binary_path.
    if language == 'glsl':
        flags = []
        if not '.noeliminate' in filename:
            flags.append('--remove-unused-variables')
        if '.legacy.' in filename:
            flags.append('--version 100 --es')
        if '.flatten.' in filename:
            flags.append('--flatten-ubo')
        if '.flatten_dim.' in filename:
            flags.append('--flatten-multidimensional-arrays')
        if '.sso.' in filename:
            flags.append('--separate-shader-objects')

        if not '.nocompat.' in filename:
            do_test(binary_path, reference_path, flags)
        if '.vk.' in filename:
            do_test(binary_path, reference_path + '.vk', flags + ['--vulkan-semantics'])
    else:
        # TODO: other languages
        skip_test()

def run_tests(binary_dir, source_dir):
    '''
    If binary_dir exists, run the tests in it, otherwise run the tests in
    source_dir, saving the compiled form to binary_dir as we go.
    '''
    if os.path.isdir(binary_dir):
        top_dir = binary_dir
        use_source = False
    else:
        if source_dir is None:
            print('Binary directory "' + binary_dir + '"not found and no source directory given',
                  file=sys.stderr)
            sys.exit(1)
        top_dir = source_dir
        use_source = True

    global test_count, pass_count
    test_count = 0
    pass_count = 0
    compiled = set()

    for test_case_dir, language, optimize in test_case_dirs:
        walk_dir = os.path.join(top_dir, test_case_dir)
        for dirpath, dirnames, filenames in os.walk(walk_dir):
            dirnames.sort()
            for filename in sorted(filenames):
                relpath = os.path.relpath(dirpath, top_dir)
                binary_path = os.path.join(binary_dir, relpath, filename)
                reference_path = os.path.join(spirv_cross_dir, 'reference', relpath, filename)
                if use_source:
                    source_path = os.path.join(source_dir, relpath, filename)
                    if source_path not in compiled:
                        compile_shader(source_path, binary_path, filename, optimize)
                        compiled.add(source_path)
                test_shader(binary_path, reference_path, filename, language, optimize)

    print(test_count, 'test cases')
    print(pass_count, 'passed')

if not 6 <= len(sys.argv) <= 8:
    print('usage:', sys.argv[0], '<spvc> <spirv-as> <spirv-opt> <glslangValidator>',
          '<SPIRV-cross directory> [binary-directory [source-directory]]', file=sys.stderr)
    sys.exit(1)

spvc_path, spirv_as_path, spirv_opt_path, glslangValidator_path, spirv_cross_dir = sys.argv[1:6]

if len(sys.argv) == 6:
    run_tests('shader_cache', spirv_cross_dir)
elif len(sys.argv) == 7:
    run_tests(sys.argv[6], None)
else:
    run_tests(sys.argv[6], sys.argv[7])

# TODO: remove the magic number once all tests pass
sys.exit(pass_count != 137)
