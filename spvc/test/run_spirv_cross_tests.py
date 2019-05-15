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

"""
Run the spirv-cross tests on spvc.
"""

from multiprocessing import Pool
import argparse
import filecmp
import os
import subprocess
import sys
import tempfile

pass_count = 0
args = None # command line arguments
devnull = None


def log_command(cmd):
    global args
    if args.log:
        # make sure it's all strings
        cmd = [str(x) for x in cmd]
        # first item is the command path, keep only last component
        cmd[0] = os.path.basename(cmd[0])
        # if last item is a path in SPIRV-Cross dir, trim that dir
        if cmd[-1].startswith(args.cross_dir):
            cmd[-1] = cmd[-1][len(args.cross_dir) + 1:]
        args.log.write(''.join(cmd) + '\n')
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
def check_reference(result, shader, optimize):
    global args
    if optimize:
        reference = os.path.join('reference', 'opt', shader)
    else:
        reference = os.path.join('reference', shader)
    log_command(['reference', reference])
    if args.dry_run or filecmp.cmp(result, os.path.join(args.cross_dir, reference), False):
        return True, reference
    elif args.give_up:
        sys.exit()
    return False, reference


# Remove files and be quiet if they don't exist or can't be removed.
def remove_files(*filenames):
    for i in filenames:
        try:
            os.remove(i)
        except:
            pass


# Prepare Vulkan binary for input to spvc.  The test input is either:
# - Vulkan text, assembled with spirv-as
# - GLSL, converted with glslang
# Optionally pass through spirv-opt.
def compile_input_shader(shader, filename, optimize):
    global args
    _, tmpfile = tempfile.mkstemp()
    shader_path = os.path.join(args.cross_dir, shader)
    if '.asm.' in filename:
        flags = ['--target-env', 'vulkan1.1']
        if '.preserve.' in filename:
            flags.append('--preserve-numeric-ids')
        spirv_as(shader_path, tmpfile, flags)
    else:
        glslang_compile(shader_path, tmpfile, ['--target-env', 'vulkan1.1', '-V'])
    if optimize:
        spirv_opt(tmpfile, tmpfile, [])
    return tmpfile


# Test spvc producing GLSL the same way SPIRV-Cross is tested.
# There are three steps: compile input, convert to GLSL, check result.
def test_glsl(shader, filename, optimize):
    global args

    input = compile_input_shader(shader, filename,
                  optimize and not '.noopt.' in filename and not '.invalid.' in filename)
    if not '.invalid.' in filename:
        # logged for compatibility with SPIRV-Cross test script
        log_command(['spirv-val', '--target-env', 'vulkan1.1', input])

    # Run spvc to convert Vulkan to GLSL.  Up to two tests are performed:
    # - Regular test on most files
    # - Vulkan-specific test on Vulkan test input
    flags = ['--entry=main', '--language=glsl']
    if not '.noeliminate' in filename:
        flags.append('--remove-unused-variables')
    if '.legacy.' in filename:
        flags.extend(['--glsl-version=100', '--es'])
    if '.flatten.' in filename:
        flags.append('--flatten-ubo')
    if '.flatten_dim.' in filename:
        flags.append('--flatten-multidimensional-arrays')
    if '.push-ubo.' in filename:
        flags.append('--glsl-emit-push-constant-as-ubo')
    if '.sso.' in filename:
        flags.append('--separate-shader-objects')

    output = None
    tests = 0
    passes = 0
    if not '.nocompat.' in filename:
        tests += 1
        output = spvc(input, input + filename , flags)
        # logged for compatibility with SPIRV-Cross test script
        log_command([args.glslang, output])

    output_vk = None
    if '.vk.' in filename or '.asm.' in filename:
        tests += 1
        output_vk = spvc(input, input + 'vk' + filename, flags + ['--vulkan-semantics'])
        # logged for compatibility with SPIRV-Cross test script
        log_command([args.glslang, '--target-env', 'vulkan1.1', '-V', output_vk])

    # Check result(s).
    # Compare either or both files produced above to appropriate reference file.
    if not '.nocompat.' in filename and output:
        result, _ = check_reference(output, shader, optimize)
        if result:
            passes += 1
    if '.vk.' in filename and output_vk:
        result, _ = check_reference(output_vk, shader + '.vk', optimize)
        if result:
            passes += 1

    remove_files(input, output, output_vk)
    return tests, passes


# Search first column of 'table' to return item from second column.
# The last item will be returned if nothing earlier matches.
def lookup(table, filename):
    for needle, haystack in zip(table[0::2], table[1::2]):
        if '.' + needle + '.' in filename:
            break
    return haystack


shader_models = (
    'sm60', '60',
    'sm51', '51',
    'sm30', '30',
    '',     '50',
)
msl_standards = (
    'msl2',  '20000',
    'msl21', '20100',
    'msl11', '10100',
    '',      '10200',
)
msl_standards_ios = (
    'msl2',  '-std=ios-metal2.0',
    'msl21', '-std=ios-metal2.1',
    'msl11', '-std=ios-metal1.1',
    'msl10', '-std=ios-metal1.0',
    '',      '-std=ios-metal1.2',
)
msl_standards_macos = (
    'msl2',  '-std=macos-metal2.0',
    'msl21', '-std=macos-metal2.1',
    'msl11', '-std=macos-metal1.1',
    '',      '-std=macos-metal1.2',
)


# Test spvc producing MSL the same way SPIRV-Cross is tested.
# There are three steps: compile input, convert to HLSL, check result.
def test_msl(shader, filename, optimize):
    global args

    input = compile_input_shader(shader, filename, optimize and not '.noopt.' in filename)

    # Run spvc to convert Vulkan to MSL.
    flags = ['--entry=main', '--language=msl', '--msl-version=' + lookup(msl_standards, filename)]
    # TODO(fjhenigman): add these flags to spvc and uncomment these lines
    #if '.swizzle.' in filename:
    #    flags.append('--msl-swizzle-texture-samples')
    #if '.ios.' in filename:
    #    flags.append('--msl-ios')
    #if '.pad-fragment.' in filename:
    #    flags.append('--msl-pad-fragment-output')
    #if '.capture.' in filename:
    #    flags.append('--msl-capture-output')
    #if '.domain.' in filename:
    #    flags.append('--msl-domain-lower-left')
    #if '.argument.' in shader:
    #    flags.append('--msl-argument-buffers')
    #if '.discrete.' in shader:
    #    flags.append('--msl-discrete-descriptor-set=2')
    #    flags.append('--msl-discrete-descriptor-set=3')

    tests = 1
    output = spvc(input, input + filename, flags)
    if not '.invalid.' in filename:
        # logged for compatibility with SPIRV-Cross test script
        log_command(['spirv-val', '--target-env', 'vulkan1.1', input])

    # Check result.
    passes = 0
    if output:
        result, reference = check_reference(output, shader, optimize)
        if result:
            passes += 1

        # logged for compatibility with SPIRV-Cross test script
        log_command(['xcrun', '--sdk',
                     'iphoneos' if '.ios.' in filename else 'macosx',
                     'metal', '-x', 'metal',
                     lookup(msl_standards_ios if '.ios.' in filename else msl_standards_macos,
                            filename),
                     '-Werror', '-Wno-unused-variable', reference])

    remove_files(input, output)
    return tests, passes


# Test spvc producing HLSL the same way SPIRV-Cross is tested.
# There are three steps: compile input, convert to HLSL, check result.
def test_hlsl(shader, filename, optimize):
    global args

    input = compile_input_shader(shader, filename, optimize and not '.noopt.' in filename)

    # Run spvc to convert Vulkan to HLSL.
    tests = 1
    output = spvc(input, input + filename, ['--entry=main', '--language=hlsl', '--hlsl-enable-compat', '--shader-model=' + lookup(shader_models, filename)])
    if not '.invalid.' in filename:
        # logged for compatibility with SPIRV-Cross test script
        log_command(['spirv-val', '--target-env', 'vulkan1.1', input])

    passes = 0
    if output:
        # logged for compatibility with SPIRV-Cross test script
        log_command([args.glslang, '-e', 'main', '-D', '--target-env', 'vulkan1.1', '-V', output])
        # TODO(fjhenigman): log fxc run here
        result, _ = check_reference(output, shader, optimize)
        if result:
            passes += 1

    remove_files(input, output)
    return tests, passes


def test_reflection(shader, filename, optimize):
    return 1, 0
    # TODO(fjhenigman)


# TODO(fjhenigman): Allow our own tests, not just spirv-cross tests.
test_case_dirs = (
    # directory             function         optimize
    ('shaders',             test_glsl,       False),
    ('shaders',             test_glsl,       True),
    ('shaders-no-opt',      test_glsl,       False),
    ('shaders-msl',         test_msl,        False),
    ('shaders-msl',         test_msl,        True),
    ('shaders-msl-no-opt',  test_msl,        False),
    ('shaders-hlsl',        test_hlsl,       False),
    ('shaders-hlsl',        test_hlsl,       True),
    ('shaders-hlsl-no-opt', test_hlsl,       False),
    ('shaders-reflection',  test_reflection, False),
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


def work_function(work_args):
    (test_function, shader, filename, optimize) = work_args
    return test_function(shader, filename, optimize)
    

def main():
    global args
    global devnull
    global pass_count

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

    pass_count = 0
    devnull = open(os.devnull, 'w')
    tests = []

    for test_case_dir, test_function, optimize in test_case_dirs:
        walk_dir = os.path.join(args.cross_dir, test_case_dir)
        for dirpath, dirnames, filenames in os.walk(walk_dir):
            dirnames.sort()
            reldir = os.path.relpath(dirpath, args.cross_dir)
            for filename in sorted(filenames):
                tests.append((test_function, os.path.join(reldir, filename), filename, optimize))

    pool = Pool()
    results = pool.map(work_function, tests)

    tests, passes = zip(*results)

    pass_count = sum(passes)
    print("{} test cases".format(sum(tests)))
    print("{} passed".format(pass_count))

    devnull.close()
    if args.log is not None and args.log is not sys.stdout:
        args.log.close()


if __name__ == "__main__":
    main()
    # TODO: remove the magic number once all tests pass
    sys.exit(pass_count != 1246)
