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

devnull = None
not_used, tmpfile = tempfile.mkstemp()

# Quietly run a command.  Throw exception on failure.
def check_call(cmd):
    global devnull
    subprocess.check_call(cmd, stdout=devnull)

# Run spirv-as.  Throw exception on failure.
def spirv_as(inp, out, flags):
    check_call([spirv_as_path] + flags + ['-o', out, inp])

# Run spirv-opt.  Throw exception on failure.
def spirv_opt(inp, out, flags):
    check_call([spirv_opt_path] + flags + ['--skip-validation', '-O', '-o', out, inp])

# Run glslangValidator as a compiler.  Throw exception on failure.
def glslang_compile(inp, out, flags):
    check_call([glslangValidator_path] + flags + ['-o', out, inp])

# Run spvc, return 'out' on success, None on failure.
def spvc(inp, out, flags):
    global devnull
    cmd = [spvc_path] + flags + ['-o', out, '--validate=vulkan1.1', inp]
    return None if subprocess.call(cmd, stdout=devnull) else out

# Compare result file to reference file and count matches.
def check_reference(result, reference):
    global pass_count
    if filecmp.cmp(result, reference, False):
        pass_count += 1

# Remove files and be quiet if they don't exist or can't be removed.
def remove_files(*filenames):
    for i in filenames:
        try:
            os.remove(i)
        except:
            pass

# Test spvc producing GLSL the same way SPIRV-Cross is tested.
# There are three steps: prepare input, convert to GLSL, check result.
def test_glsl(shader, filename, optimize):
    global spirv_cross_dir, tmpfile
    global test_count
    shader_path = os.path.join(spirv_cross_dir, shader)

    # Prepare Vulkan binary.  The test input is either:
    # - Vulkan text, assembled with spirv-as
    # - GLSL, converted with glslang
    # Optionally pass through spirv-opt.
    if '.asm.' in filename:
        flags = []
        if '.preserve.' in filename:
            flags.append('--preserve-numeric-ids')
        spirv_as(shader_path, tmpfile, flags)
    else:
        glslang_compile(shader_path, tmpfile, ['--target-env', 'vulkan1.1', '-V'])
    if optimize and not '.noopt.' in filename and not '.invalid.' in filename:
        spirv_opt(tmpfile, tmpfile, [])

    # Run spvc to convert Vulkan to GLSL.  Up to two tests are performed:
    # - Regular test on most files
    # - Vulkan-specific test on Vulkan test input
    flags = []
    output = None
    if not '.nocompat.' in filename:
        test_count += 1
        output = spvc(tmpfile, tmpfile + filename , flags)
    output_vk = None
    if '.vk.' in filename or '.asm.' in filename:
        #TODO(fjhenigman): add Vulkan-specific flags.
        test_count += 1
        output_vk = spvc(tmpfile, tmpfile + 'vk' + filename, flags)

    # Check results.
    # Compare either or both files produced above to appropriate reference file.
    if optimize:
        reference = os.path.join(spirv_cross_dir, 'reference', 'opt', shader)
    else:
        reference = os.path.join(spirv_cross_dir, 'reference', shader)
    if not '.nocompat.' in filename and output:
        check_reference(output, reference)
    if '.vk.' in filename and output_vk:
        check_reference(output_vk, reference + '.vk')

    # Clean up.
    remove_files(tmpfile, output, output_vk)

def main(shader_dir):
    global devnull
    global test_count, pass_count
    test_count = 0
    pass_count = 0
    devnull = open(os.devnull, 'w')

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
