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
'''

from __future__ import print_function

import errno
import filecmp
import os
import subprocess
import sys
import tempfile

# TODO(fjhenigman): Allow our own tests, not just spirv-cross tests.
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

def check_call(cmd):
    global devnull
    if subprocess.call(cmd, stdout=devnull):
        print('failed command: ', ' '.join(cmd))
        sys.exit(1)

def spirv_as(inp, out, flags):
    check_call([spirv_as_path] + flags + ['-o', out, inp])

def spirv_opt(inp, out, flags):
    check_call([spirv_opt_path] + flags + ['--skip-validation', '-O', '-o', out, inp])

def glslang_compile(inp, out, flags):
    check_call([glslangValidator_path] + flags + ['-o', out, inp])

def spvc(inp, out, flags):
    global devnull
    global test_count
    test_count += 1
    cmd = [spvc_path] + flags + ['-o', out, '--validate=vulkan1.1', inp]
    return subprocess.call(cmd, stdout=devnull)

def check_reference(result, reference):
    global spirv_cross_dir
    global pass_count
    #log('reference', reference)
    if result and filecmp.cmp(result, os.path.join(spirv_cross_dir, reference), False):
        pass_count += 1

def test_glsl(shader, filename, optimize):
    global spirv_cross_dir
    shader_path = os.path.join(spirv_cross_dir, shader)
    temp = '/tmp/cross-'

    # compile shader
    if '.asm.' in filename:
        flags = []
        if '.preserve.' in filename:
            flags.append('--preserve-numeric-ids')
        spirv_as(shader_path, temp, flags)
    else:
        glslang_compile(shader_path, temp, ['--target-env', 'vulkan1.1', '-V'])
    if optimize and not '.noopt.' in filename and not '.invalid.' in filename:
        spirv_opt(temp, temp, [])

    # run spvc
    flags = []
    if not '.nocompat.' in filename:
        output = temp + filename
        if spvc(temp, output, flags):
            output = ""
    if '.vk.' in filename or '.asm.' in filename:
        output_vk = temp + 'vk' + filename
        if spvc(temp, output_vk, flags):
            output_vk = ""

    # check result
    reference = ['reference', shader]
    if optimize:
        reference.insert(1, 'opt')
    reference = os.path.join(*reference)
    if not '.nocompat.' in filename:
        check_reference(output, reference)
    if '.vk.' in filename:
        check_reference(output_vk, reference + '.vk')

def main(shader_dir):
    global devnull
    devnull = open(os.devnull, 'w')

    global test_count, pass_count
    test_count = 0
    pass_count = 0

    for test_case_dir, language, optimize in test_case_dirs:
        walk_dir = os.path.join(shader_dir, test_case_dir)
        for dirpath, dirnames, filenames in os.walk(walk_dir):
            dirnames.sort()
            reldir = os.path.relpath(dirpath, shader_dir)
            for filename in sorted(filenames):
                if language == 'glsl':
                    test_glsl(os.path.join(reldir, filename), filename, bool(optimize))
                else:
                    test_count += 1

    print(test_count, 'test cases')
    print(pass_count, 'passed')

    devnull.close()

if len(sys.argv) != 6:
    print('usage:', sys.argv[0], '<spvc> <spirv-as> <spirv-opt> <glslangValidator>',
          '<SPIRV-cross directory>', file=sys.stderr)
    sys.exit(1)

spvc_path, spirv_as_path, spirv_opt_path, glslangValidator_path, spirv_cross_dir = sys.argv[1:6]
main(spirv_cross_dir)

# TODO: remove the magic number once all tests pass
sys.exit(pass_count != 169)
