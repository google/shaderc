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

"""Run the spirv-cross tests on spvc."""

from multiprocessing import Pool
import argparse
import filecmp
import os
import re
import subprocess
import sys
import tempfile


def log_command(script_args, cmd):
    if script_args.log:
        # make sure it's all strings
        cmd = [str(x) for x in cmd]
        # first item is the command path, keep only last component
        cmd[0] = os.path.basename(cmd[0])
        # if last item is a path in SPIRV-Cross dir, trim that dir
        if cmd[-1].startswith(script_args.cross_dir):
            cmd[-1] = cmd[-1][len(script_args.cross_dir) + 1:]
        script_args.log.write(''.join(cmd) + '\n')
        script_args.log.flush()


# Quietly run a command.  Throw exception on failure.
def check_call(script_args, cmd):
    log_command(script_args, cmd)
    if not script_args.dry_run:
        subprocess.check_call(cmd, stdout=subprocess.DEVNULL)


# Run spirv-as.  Throw exception on failure.
def spirv_as(script_args, inp, out, flags):
    check_call(script_args, [script_args.spirv_as] + flags + ['-o', out, inp])


# Run spirv-opt.  Throw exception on failure.
def spirv_opt(script_args, inp, out, flags):
    check_call(script_args, [script_args.spirv_opt] +
               flags + ['--skip-validation', '-O', '-o', out, inp])


# Run glslangValidator as a compiler.  Throw exception on failure.
def glslang_compile(script_args, inp, out, flags):
    check_call(script_args, [script_args.glslang] + flags + ['-o', out, inp])


# Run spvc, return 'out' on success, None on failure.
def spvc(script_args, inp, out, flags):
    cmd = [script_args.spvc] + flags + ['-o', out, '--validate=vulkan1.1', inp]
    log_command(script_args, cmd)
    if script_args.dry_run or subprocess.call(cmd, stdout=subprocess.DEVNULL) == 0:
        return out
    if script_args.give_up:
        sys.exit()


# Compare result file to reference file and count matches. Returns the result of
# the comparison and the reference file being used. Exits if |give_up| set on
# failure.
def check_reference(script_args, result, shader, optimize):
    if optimize:
        reference = os.path.join('reference', 'opt', shader)
    else:
        reference = os.path.join('reference', shader)
    log_command(script_args, ['reference', reference])
    if script_args.dry_run or filecmp.cmp(result, os.path.join(script_args.cross_dir, reference), False):
        return True, reference
    elif script_args.give_up:
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
# Returns the temp file that the shader was compiled to.
def compile_input_shader(script_args, shader, filename, optimize):
    _, tmpfile = tempfile.mkstemp()
    shader_path = os.path.join(script_args.cross_dir, shader)
    if '.asm.' in filename:
        flags = ['--target-env', 'vulkan1.1']
        if '.preserve.' in filename:
            flags.append('--preserve-numeric-ids')
        spirv_as(script_args, shader_path, tmpfile, flags)
    else:
        glslang_compile(script_args, shader_path, tmpfile, [
                        '--target-env', 'vulkan1.1', '-V'])
    if optimize:
        spirv_opt(script_args, tmpfile, tmpfile, [])
    return tmpfile


# Test spvc producing GLSL the same way SPIRV-Cross is tested.
# There are three steps: compile input, convert to GLSL, check result.
# Returns the number of tests run and the number of them that passed.
def test_glsl(script_args, shader, filename, optimize):
    input = compile_input_shader(script_args, shader, filename,
                                 optimize and not '.noopt.' in filename and not '.invalid.' in filename)
    if not '.invalid.' in filename:
        # logged for compatibility with SPIRV-Cross test script
        log_command(script_args, ['spirv-val',
                                  '--target-env', 'vulkan1.1', input])

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
        output = spvc(script_args, input, input + filename, flags)
        # logged for compatibility with SPIRV-Cross test script
        log_command(script_args, [script_args.glslang, output])

    output_vk = None
    if '.vk.' in filename or '.asm.' in filename:
        tests += 1
        output_vk = spvc(script_args, input, input + 'vk' +
                         filename, flags + ['--vulkan-semantics'])
        # logged for compatibility with SPIRV-Cross test script
        log_command(script_args, [script_args.glslang,
                                  '--target-env', 'vulkan1.1', '-V', output_vk])

    # Check result(s).
    # Compare either or both files produced above to appropriate reference file.
    if not '.nocompat.' in filename and output:
        result, _ = check_reference(script_args, output, shader, optimize)
        if result:
            passes += 1
    if '.vk.' in filename and output_vk:
        result, _ = check_reference(
            script_args, output_vk, shader + '.vk', optimize)
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
# Returns the number of tests run and the number of them that passed.
def test_msl(script_args, shader, filename, optimize):
    input = compile_input_shader(
        script_args, shader, filename, optimize and not '.noopt.' in filename)

    # Run spvc to convert Vulkan to MSL.
    flags = ['--entry=main', '--language=msl',
             '--msl-version=' + lookup(msl_standards, filename)]
    # TODO(fjhenigman): add these flags to spvc and uncomment these lines
    # if '.swizzle.' in filename:
    #    flags.append('--msl-swizzle-texture-samples')
    # if '.ios.' in filename:
    #    flags.append('--msl-ios')
    # if '.pad-fragment.' in filename:
    #    flags.append('--msl-pad-fragment-output')
    # if '.capture.' in filename:
    #    flags.append('--msl-capture-output')
    # if '.domain.' in filename:
    #    flags.append('--msl-domain-lower-left')
    # if '.argument.' in shader:
    #    flags.append('--msl-argument-buffers')
    # if '.discrete.' in shader:
    #    flags.append('--msl-discrete-descriptor-set=2')
    #    flags.append('--msl-discrete-descriptor-set=3')

    tests = 1
    output = spvc(script_args, input, input + filename, flags)
    if not '.invalid.' in filename:
        # logged for compatibility with SPIRV-Cross test script
        log_command(script_args, ['spirv-val',
                                  '--target-env', 'vulkan1.1', input])

    # Check result.
    passes = 0
    if output:
        result, reference = check_reference(
            script_args, output, shader, optimize)
        if result:
            passes += 1

        # logged for compatibility with SPIRV-Cross test script
        log_command(script_args, ['xcrun', '--sdk',
                                  'iphoneos' if '.ios.' in filename else 'macosx',
                                  'metal', '-x', 'metal',
                                  lookup(msl_standards_ios if '.ios.' in filename else msl_standards_macos,
                                         filename),
                                  '-Werror', '-Wno-unused-variable', reference])

    remove_files(input, output)
    return tests, passes


# Test spvc producing HLSL the same way SPIRV-Cross is tested.
# There are three steps: compile input, convert to HLSL, check result.
# Returns the number of tests run and the number of them that passed.
def test_hlsl(script_args, shader, filename, optimize):
    input = compile_input_shader(
        script_args, shader, filename, optimize and not '.noopt.' in filename)

    # Run spvc to convert Vulkan to HLSL.
    tests = 1
    output = spvc(script_args, input, input + filename,
                  ['--entry=main', '--language=hlsl', '--hlsl-enable-compat', '--shader-model=' + lookup(shader_models, filename)])
    if not '.invalid.' in filename:
        # logged for compatibility with SPIRV-Cross test script
        log_command(script_args, ['spirv-val',
                                  '--target-env', 'vulkan1.1', input])

    passes = 0
    if output:
        # logged for compatibility with SPIRV-Cross test script
        log_command(script_args, [script_args.glslang, '-e', 'main',
                                  '-D', '--target-env', 'vulkan1.1', '-V', output])
        # TODO(fjhenigman): log fxc run here
        result, _ = check_reference(script_args, output, shader, optimize)
        if result:
            passes += 1

    remove_files(input, output)
    return tests, passes


# Currently a no-op test. Needs to be implemented.
def test_reflection(script_args, shader, filename, optimize):
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
    (test_function, script_args, shader, filename, optimize) = work_args
    return test_function(script_args, shader, filename, optimize)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--log', action=FileArgAction,
                        help='log commands to file')
    parser.add_argument('-n', '--dry-run', dest='dry_run', action='store_true',
                        help='do not execute commands')
    parser.add_argument('-g', '--give-up', dest='give_up', action='store_true',
                        help='quit after first failure')
    parser.add_argument('-f', '--test-filter', dest='test_filter',
                        action='store', metavar='<test filter regex>',
                        help='only run tests that contain given regex string')
    parser.add_argument('spvc', metavar='<spvc executable>')
    parser.add_argument('spirv_as', metavar='<spirv-as executable>')
    parser.add_argument('spirv_opt', metavar='<spirv-opt executable>')
    parser.add_argument('glslang', metavar='<glslangValidator executable>')
    parser.add_argument('cross_dir', metavar='<SPIRV-cross directory>')
    script_args = parser.parse_args()

    test_regex = None
    if script_args.test_filter:
        print('Filtering tests using \'{}\''.format(script_args.test_filter))
        test_regex = re.compile(script_args.test_filter)

    tests = []
    for test_case_dir, test_function, optimize in test_case_dirs:
        walk_dir = os.path.join(script_args.cross_dir, test_case_dir)
        for dirpath, dirnames, filenames in os.walk(walk_dir):
            dirnames.sort()
            reldir = os.path.relpath(dirpath, script_args.cross_dir)
            for filename in sorted(filenames):
                shader = os.path.join(reldir, filename)
                if not test_regex or re.search(test_regex, shader):
                    tests.append((test_function, script_args,
                                  shader, filename, optimize))

    pool = Pool()
    results = pool.map(work_function, tests)

    tests, passes = zip(*results)

    pass_count = sum(passes)
    print('{} test cases'.format(sum(tests)))
    print('{} passed'.format(pass_count))

    if script_args.log is not None and script_args.log is not sys.stdout:
        script_args.log.close()
    return pass_count


if __name__ == '__main__':
    # TODO: remove the magic number once all tests pass
    sys.exit(main() != 1246)
