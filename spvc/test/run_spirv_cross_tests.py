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

import argparse
import errno
import filecmp
import os
import subprocess
import sys
import tempfile

test_count = 0
pass_count = 0

args = None # command line arguments
not_used, tmpfile = tempfile.mkstemp()
devnull = None

def log_command(cmd):
    global args
    if args.log:
        # make sure it's all strings
        cmd = map(str, cmd)
        # first item is the command path, keep only last component
        cmd[0] = os.path.basename(cmd[0])
        # if last item is a path in SPIRV-Cross dir, trim that dir
        if cmd[-1].startswith(args.cross_dir):
            cmd[-1] = cmd[-1][len(args.cross_dir) + 1:]
        print(' '.join(cmd), file=args.log)
        args.log.flush()

# Quietly run a command.  Throw exception on failure.
def check_call(cmd):
    global args
    global devnull
    log_command(cmd)
    if not args.dry_run:
        subprocess.check_call(cmd, stdout=devnull)

# Run spirv-as.  Throw exception on failure.
def spirv_as(inp, out, flags):
    global args
    check_call([args.spirv_as] + flags + ['-o', out, inp])

# Run spirv-opt.  Throw exception on failure.
def spirv_opt(inp, out, flags):
    global args
    check_call([args.spirv_opt] + flags + ['--skip-validation', '-O', '-o', out, inp])

# Run glslangValidator as a compiler.  Throw exception on failure.
def glslang_compile(inp, out, flags):
    global args
    check_call([args.glslang] + flags + ['-o', out, inp])

# Run spvc, return 'out' on success, None on failure.
def spvc(inp, out, flags):
    global args
    global devnull
    cmd = [args.spvc] + flags + ['-o', out, '--validate=vulkan1.1', inp]
    log_command(cmd)
    if args.dry_run or subprocess.call(cmd, stdout=devnull) == 0:
        return out
    if args.give_up:
        sys.exit()

# Compare result file to reference file and count matches.
def check_reference(result, reference):
    global args
    global pass_count
    log_command(['reference', reference])
    if args.dry_run or filecmp.cmp(result, os.path.join(args.cross_dir, reference), False):
        pass_count += 1
    elif args.give_up:
        sys.exit()

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
    global args
    global tmpfile
    global test_count
    shader_path = os.path.join(args.cross_dir, shader)

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
    if not '.invalid.' in filename:
        # logged for compatibility with SPIRV-Cross test script
        log_command(['spirv-val', '--target-env', 'vulkan1.1', tmpfile])

    # Run spvc to convert Vulkan to GLSL.  Up to two tests are performed:
    # - Regular test on most files
    # - Vulkan-specific test on Vulkan test input
    flags = ['--entry=main']
    if not '.noeliminate' in filename:
        flags.append('--remove-unused-variables')
    if '.legacy.' in filename:
        flags.extend(['--version=100', '--es'])
    if '.flatten.' in filename:
        flags.append('--flatten-ubo')
    if '.flatten_dim.' in filename:
        flags.append('--flatten-multidimensional-arrays')
    if '.sso.' in filename:
        flags.append('--separate-shader-objects')

    output = None
    if not '.nocompat.' in filename:
        test_count += 1
        output = spvc(tmpfile, tmpfile + filename , flags)
        # logged for compatibility with SPIRV-Cross test script
        log_command([args.glslang, output])

    output_vk = None
    if '.vk.' in filename or '.asm.' in filename:
        test_count += 1
        output_vk = spvc(tmpfile, tmpfile + 'vk' + filename, flags + ['--vulkan-semantics'])
        # logged for compatibility with SPIRV-Cross test script
        log_command([args.glslang, '--target-env', 'vulkan1.1', '-V', output_vk])

    # Check results.
    # Compare either or both files produced above to appropriate reference file.
    if optimize:
        reference = os.path.join('reference', 'opt', shader)
    else:
        reference = os.path.join('reference', shader)
    if not '.nocompat.' in filename and output:
        check_reference(output, reference)
    if '.vk.' in filename and output_vk:
        check_reference(output_vk, reference + '.vk')

    # Clean up.
    remove_files(tmpfile, output, output_vk)

# Stub for tests not yet implemented.
def test_todo(shader, filename, optimize):
    global test_count
    test_count += 1

# TODO(fjhenigman): Allow our own tests, not just spirv-cross tests.
test_case_dirs = (
# directory             function    args
('shaders'            , test_glsl, {'optimize':False}),
('shaders'            , test_glsl, {'optimize':True }),
('shaders-no-opt'     , test_glsl, {'optimize':False}),
('shaders-msl'        , test_todo, {'optimize':False}),
('shaders-msl'        , test_todo, {'optimize':True }),
('shaders-msl-no-opt' , test_todo, {'optimize':False}),
('shaders-hlsl'       , test_todo, {'optimize':False}),
('shaders-hlsl'       , test_todo, {'optimize':True }),
('shaders-hlsl-no-opt', test_todo, {'optimize':False}),
('shaders-reflection' , test_todo, {'optimize':False}),
)

class FileArgAction(argparse.Action):
    def __call__(self, parser, namespace, value, option):
        if value == '-':
            log = sys.stdout
        else:
            try:
                log = open(value, 'w')
            except:
                print("could not open log file '%s' for writing" % value)
                raise
        setattr(namespace, self.dest, log)

def main():
    global args
    global devnull
    global test_count, pass_count

    parser = argparse.ArgumentParser()
    parser.add_argument('--log', action=FileArgAction, help='log commands to file')
    parser.add_argument('-n', '--dry-run', dest='dry_run', action='store_true',
                        help = 'do not execute commands')
    parser.add_argument('-g', '--give-up', dest='give_up', action='store_true',
                        help = 'quit after first failure')
    parser.add_argument('spvc', metavar='<spvc executable>')
    parser.add_argument('spirv_as', metavar='<spirv-as executable>')
    parser.add_argument('spirv_opt', metavar='<spirv-opt executable>')
    parser.add_argument('glslang', metavar='<glslangValidator executable>')
    parser.add_argument('cross_dir', metavar='<SPIRV-cross directory>')
    args = parser.parse_args()

    test_count = 0
    pass_count = 0
    devnull = open(os.devnull, 'w')

    for test_case_dir, function, function_args in test_case_dirs:
        walk_dir = os.path.join(args.cross_dir, test_case_dir)
        for dirpath, dirnames, filenames in os.walk(walk_dir):
            dirnames.sort()
            reldir = os.path.relpath(dirpath, args.cross_dir)
            for filename in sorted(filenames):
                function(os.path.join(reldir, filename), filename, **function_args)

    print(test_count, 'test cases')
    print(pass_count, 'passed')
    devnull.close()
    if args.log is not None and args.log is not sys.stdout:
        args.log.close()

main()

# TODO: remove the magic number once all tests pass
sys.exit(pass_count != 526)
