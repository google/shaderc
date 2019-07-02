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
import itertools
import os
import re
import subprocess
import sys
import tempfile


class TestEnv:
    """Container for cross-test environmental data and operations."""

    def __init__(self, script_args):
        """Takes in the output of ArgumentParser.parse_args()"""
        self.dry_run = script_args.dry_run
        self.verbose = script_args.verbose
        self.give_up = script_args.give_up
        self.cross_dir = script_args.cross_dir
        self.spvc = script_args.spvc
        self.spirv_as = script_args.spirv_as
        self.spirv_opt = script_args.spirv_opt
        self.glslang = script_args.glslang

    def log_unexpected_successes(self, successes):
        """Log list of unexpected test case successes."""
        if not len(successes):
            log_string = 'Encountered 0 unexpected successes'
        else:
            log_string = 'Encountered {} unexpected success(es):\n'.format(
                len(successes))
            successes = ['\t{}'.format(success) for success in successes]
            log_string += '\n'.join(successes)
        print(log_string)

    def log_unexpected_failures(self, failures):
        """Log list of unexpected test case failures."""
        if not len(failures):
            log_string = 'Encountered 0 unexpected failures'
        else:
            log_string = 'Encountered {} unexpected failures(s):\n'.format(
                len(failures))
            failures = ['\t{}'.format(failure) for failure in failures]
            log_string += '\n'.join(failures)
        print(log_string)

    def log_failure(self, shader, optimize):
        """Log a test case failure."""
        if self.verbose:
            log_string = 'FAILED {}, optimize = {}'.format(shader, optimize)
            print(log_string)

    def log_command(self, cmd):
        """Log calling a command."""
        if self.verbose:
            # make sure it's all strings
            cmd = [str(x) for x in cmd]
            # first item is the command path, keep only last component
            cmd[0] = os.path.basename(cmd[0])
            # if last item is a path in SPIRV-Cross dir, trim that dir
            if cmd[-1].startswith(self.cross_dir):
                cmd[-1] = cmd[-1][len(self.cross_dir) + 1:]
            log_string = ' '.join(cmd) + '\n'
            print(log_string)

    def check_output(self, cmd):
        """Quietly run a command.

        Returns status of |cmd|, output of |cmd|.
        """
        self.log_command(cmd)
        if self.dry_run:
            return True, None

        try:
            out = subprocess.check_output(cmd)
            return True, out
        except subprocess.SubprocessError as e:
            return False, e.output

    def run_spirv_as(self, inp, out, flags):
        """Run spirv-as.

        Returns status of spirv-as, output of spirv-as.
        """
        return self.check_output([self.spirv_as] + flags + ['-o', out, inp])

    def run_spirv_opt(self, inp, out, flags):
        """Run spirv-opt.

        Returns status of spirv-out, output of spirv-out.
        """
        return self.check_output([self.spirv_opt] + flags + ['--skip-validation', '-O', '-o', out, inp])

    def run_glslang_compile(self, inp, out, flags):
        """Run glslangValidator as a compiler.

        Returns status of glslangValidator, output of glslangValidator.
        """
        return self.check_output([self.glslang] + flags + ['-o', out, inp])

    def run_spvc(self, inp, out, flags):
        """Run spvc.

        Returns status of spvc, output of spvc. Exits entirely if spvc
        fails and give_up flag is set.
        """
        status, output = self.check_output(
            [self.spvc] + flags + ['-o', out, '--source-env=vulkan1.1', '--target-env=vulkan1.1', inp])
        if not status and self.give_up:
            print('Bailing due to failure in run_spvc with give_up set')
            sys.exit()
        return status, output

    def check_reference(self, result, shader, optimize):
        """Compare result file to reference file and count matches.

        Returns the result of the comparison and the reference file
        being used. Exits entirely if spvc fails and give_up flag is
        set.
        """
        if optimize:
            reference = os.path.join('reference', 'opt', shader)
        else:
            reference = os.path.join('reference', shader)
        self.log_command(['reference', reference])
        if self.dry_run or filecmp.cmp(
                result, os.path.join(self.cross_dir, reference), False):
            return True, reference
        elif self.give_up:
            print('Bailing due to failure in check_reference with give_up set')
            sys.exit()

        return False, reference

    def compile_input_shader(self, shader, filename, optimize):
        """Prepare Vulkan binary for input to spvc.

        The test input is either:
            - Vulkan text, assembled with spirv-as
            - GLSL, converted with glslang
        Optionally pass through spirv-opt.
        Returns the status of the operation, and the temp file that the shader
        was compiled to.
        """
        _, tmpfile = tempfile.mkstemp()
        shader_path = os.path.join(self.cross_dir, shader)
        if '.asm.' in filename:
            flags = ['--target-env', 'vulkan1.1']
            if '.preserve.' in filename:
                flags.append('--preserve-numeric-ids')
            result, _ = self.run_spirv_as(shader_path, tmpfile, flags)
        else:
            result, _ = self.run_glslang_compile(shader_path, tmpfile, [
                '--target-env', 'vulkan1.1', '-V'])
        if optimize:
            result, _ = self.run_spirv_opt(tmpfile, tmpfile, [])
        return result, tmpfile


def remove_files(*filenames):
    """Remove files and be quiet if they don't exist or can't be removed."""
    for i in filenames:
        try:
            os.remove(i)
        except:
            pass


def test_glsl(test_env, shader, filename, optimize):
    """Test spvc producing GLSL the same way SPIRV-Cross is tested.

    There are three steps: compile input, convert to GLSL, check result.

    Returns a list of successful tests and a list of failed tests.
    """
    successes = []
    failures = []

    status, input_shader = test_env.compile_input_shader(
        shader, filename, optimize and not '.noopt.' in filename and not '.invalid.' in filename)
    if not status:
        remove_files(input_shader)
        failures.append((shader, optimize))
        test_env.log_failure(shader, optimize)
        return successes, failures

    if not '.invalid.' in filename:
        # logged for compatibility with SPIRV-Cross test script
        test_env.log_command(
            ['spirv-val', '--target-env', 'vulkan1.1', input_shader])

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
    if not '.nocompat.' in filename:
        output = input_shader + filename
        status, _ = test_env.run_spvc(input_shader, output, flags)
        if not status:
            output = None
        # logged for compatibility with SPIRV-Cross test script
        test_env.log_command([test_env.glslang, output])

    output_vk = None
    if '.vk.' in filename:
        output_vk = input_shader + 'vk' + filename
        status, _ = test_env.run_spvc(
            input_shader, output_vk, flags + ['--vulkan-semantics'])
        if not status:
            output_vk = None
        # logged for compatibility with SPIRV-Cross test script

    # Check result(s).
    # Compare either or both files produced above to appropriate reference
    # file.
    if not '.nocompat.' in filename:
        if output:
            result, _ = test_env.check_reference(output, shader, optimize)
            if result:
                successes.append((shader, optimize))
            else:
                failures.append((shader, optimize))
                test_env.log_failure(shader, optimize)
        else:
            failures.append((shader, optimize))
            test_env.log_failure(shader, optimize)

    if '.vk.' in filename:
        if output_vk:
            result, _ = test_env.check_reference(
                output_vk, shader + '.vk', optimize)
            if result:
                successes.append((shader + '.vk', optimize))
            else:
                failures.append((shader + '.vk', optimize))
                test_env.log_failure(shader + '.vk', optimize)
        else:
            failures.append((shader + '.vk', optimize))
            test_env.log_failure(shader + '.vk', optimize)

    remove_files(input_shader, output, output_vk)
    return successes, failures


def lookup(table, filename):
    """Search first column of 'table' to return item from second column.

    The last item will be returned if nothing earlier matches.
    """
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


def test_msl(test_env, shader, filename, optimize):
    """Test spvc producing MSL the same way SPIRV-Cross is tested.

    There are three steps: compile input, convert to HLSL, check result.

    Returns a list of successful tests and a list of failed tests.
    """
    successes = []
    failures = []
    status, input_shader = test_env.compile_input_shader(
        shader, filename, optimize and not '.noopt.' in filename)
    if not status:
        remove_files(input_shader)
        failures.append((shader, optimize))
        test_env.log_failure(shader, optimize)
        return successes, failures

    # Run spvc to convert Vulkan to MSL.
    flags = ['--entry=main', '--language=msl',
             '--msl-version=' + lookup(msl_standards, filename)]
    if '.swizzle.' in filename:
        flags.append('--msl-swizzle-texture-samples')
    if '.ios.' in filename:
        flags.append('--msl-platform=ios')
    if '.pad-fragment.' in filename:
        flags.append('--msl-pad-fragment-output')
    if '.capture.' in filename:
        flags.append('--msl-capture-output')
    if '.domain.' in filename:
        flags.append('--msl-domain-lower-left')
    if '.argument.' in shader:
        flags.append('--msl-argument-buffers')
    if '.discrete.' in shader:
        flags.append('--msl-discrete-descriptor-set=2')
        flags.append('--msl-discrete-descriptor-set=3')

    output = input_shader + filename
    status, _ = test_env.run_spvc(input_shader, output, flags)
    if not status:
        remove_files(input_shader)
        failures.append((shader, optimize))
        test_env.log_failure(shader, optimize)
        return successes, failures

    if not '.invalid.' in filename:
        # logged for compatibility with SPIRV-Cross test script
        test_env.log_command(
            ['spirv-val', '--target-env', 'vulkan1.1', input_shader])

    # Check result.
    if output:
        result, reference = test_env.check_reference(output, shader, optimize)
        if result:
            successes.append((shader, optimize))
        else:
            failures.append((shader, optimize))
            test_env.log_failure(shader, optimize)
        # logged for compatibility with SPIRV-Cross test script
        test_env.log_command(['xcrun', '--sdk',
                              'iphoneos' if '.ios.' in filename else 'macosx',
                              'metal', '-x', 'metal',
                              lookup(
                                  msl_standards_ios if '.ios.' in filename else msl_standards_macos,
                                  filename),
                              '-Werror', '-Wno-unused-variable',
                              reference])
    else:
        failures.append((shader, optimize))
        test_env.log_failure(shader, optimize)

    remove_files(input_shader, output)
    return successes, failures


def test_hlsl(test_env, shader, filename, optimize):
    """Test spvc producing HLSL the same way SPIRV-Cross is tested.

    There are three steps: compile input, convert to HLSL, check result.

    Returns a list of successful tests and a list of failed tests.
    """
    successes = []
    failures = []

    status, input_shader = test_env.compile_input_shader(
        shader, filename, optimize and not '.noopt.' in filename)
    if not status:
        remove_files(input_shader)
        failures.append((shader, optimize))
        test_env.log_failure(shader, optimize)
        return successes, failures

    # Run spvc to convert Vulkan to HLSL.
    output = input_shader + filename
    status, _ = test_env.run_spvc(input_shader, output, ['--entry=main', '--language=hlsl',
                                                         '--hlsl-enable-compat', '--shader-model=' + lookup(shader_models, filename)])
    if not status:
        remove_files(input_shader)
        failures.append((shader, optimize))
        test_env.log_failure(shader, optimize)
        return successes, failures

    if not '.invalid.' in filename:
        # logged for compatibility with SPIRV-Cross test script
        test_env.log_command(
            ['spirv-val', '--target-env', 'vulkan1.1', input_shader])

    if output:
        # logged for compatibility with SPIRV-Cross test script
        test_env.log_command([test_env.glslang, '-e', 'main',
                              '-D', '--target-env', 'vulkan1.1', '-V', output])
        # TODO(bug 649): Log dxc run here
        result, _ = test_env.check_reference(output, shader, optimize)
        if result:
            successes.append((shader, optimize))
        else:
            failures.append((shader, optimize))
            test_env.log_failure(shader, optimize)
    else:
        failures.append((shader, optimize))
        test_env.log_failure(shader, optimize)

    remove_files(input_shader, output)
    return successes, failures


def test_reflection(test_env, shader, filename, optimize):
    """Currently a no-op test. Needs to be implemented.

    Returns a tuple indicating the passed in test has failed.
    """
    test_env.log_failure(shader, optimize)
    return [], [(shader, optimize)]
    # TODO(bug 650): Implement this test


# TODO(bug 651): Allow our own tests, not just spirv-cross tests.
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


def work_function(work_args):
    """"Unpacks the test case args and invokes the appropriate in test
    function."""
    (test_function, test_env, shader, filename, optimize) = work_args
    return test_function(test_env, shader, filename, optimize)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                        help='Enable additional diagnoistic logging')
    parser.add_argument('-n', '--dry-run', dest='dry_run', action='store_true',
                        help='Do not execute commands')
    parser.add_argument('-g', '--give-up', dest='give_up',
                        action='store_true',
                        help='Quit after first failure')
    parser.add_argument('-f', '--test-filter', dest='test_filter',
                        action='store', metavar='<test filter regex>',
                        help='Only run tests that contain given regex string')
    parser.add_argument('-j', '--jobs', dest='jobs', type=int, default=0, action='store',
                        metavar='<number of processes to use>', help='Use as many processes as specified, 0 indicates let the script decide.')
    parser.add_argument('spvc', metavar='<spvc executable>')
    parser.add_argument('spirv_as', metavar='<spirv-as executable>')
    parser.add_argument('spirv_opt', metavar='<spirv-opt executable>')
    parser.add_argument('glslang', metavar='<glslangValidator executable>')
    parser.add_argument('cross_dir', metavar='<SPIRV-cross directory>')
    script_args = parser.parse_args()

    test_env = TestEnv(script_args)

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
                    tests.append((test_function, test_env,
                                  shader, filename, optimize))

    if not script_args.jobs:
        pool = Pool()
    else:
        pool = Pool(script_args.jobs)
    results = pool.map(work_function, tests)

    successes, failures = zip(*results)
    # Flattening lists of lists, and convert path markers if needed
    successes = list(itertools.chain.from_iterable(successes))
    successes = list(
        map(lambda x: (x[0].replace(os.path.sep, '/'), x[1]), successes))
    failures = list(itertools.chain.from_iterable(failures))
    failures = list(
        map(lambda x: (x[0].replace(os.path.sep, '/'), x[1]), failures))

    print('{} test cases'.format(len(successes) + len(failures)))
    print('{} passed'.format(len(successes)))

    fail_file = os.path.join(os.path.dirname(
        os.path.realpath(__file__)), 'known_failures')
    with open(fail_file, 'r') as f:
        known_failures = f.read().splitlines()

    known_failures = set(
        map(lambda x: (x.split(',')[0], x.split(',')[1] == 'True'), known_failures))

    unexpected_successes = []
    unexpected_failures = []

    for success in successes:
        if success in known_failures:
            unexpected_successes.append(success)

    for failure in failures:
        if failure not in known_failures:
            unexpected_failures.append(failure)

    test_env.log_unexpected_successes(unexpected_successes)
    test_env.log_unexpected_failures(unexpected_failures)

    return len(unexpected_successes) != 0 or len(unexpected_failures) != 0


if __name__ == '__main__':
    sys.exit(main())
