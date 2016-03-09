#!/usr/bin/env python

# Copyright 2016 The Shaderc Authors. All rights reserved.
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

"""Builds the Shaderc project, on Linux, Mac, or Windows.
"""

from __future__ import print_function
import argparse
import os
import platform
import subprocess


OS = platform.system()


def run(cmd, cwd, justprint):
    """Prints a command to run, and optionally runs it.

    Raises a RuntimeError if the command does not launch or otherwise fails.

    Args:
      justprint: If true, then only print the command. Otherwise run the 
                   command after printing it.
      cmd:       List of words in the command.
      cwd:       Working directory for the command.
    """
    print(cmd)
    if justprint:
        return

    p = subprocess.Popen(cmd, cwd=cwd)
    (_, _) = p.communicate()
    if p.returncode != 0:
        raise RuntimeError('Failed to run %s in %s' % (cmd, cwd))


def quote(string):
    return '"%s"' % string.replace('"', '\\"')


def quote_some(command, indices):
    """Transforms command (a list of strings) into a single string by
    joining its elements, but with quotes around those elements in the
    indices list.
    """
    quoted = [(quote(e) if i in indices else e)
              for (i, e) in enumerate(command)]
    return ' '.join(quoted)


def build(args):
    """ Builds Shaderc under specified conditions.

    Args:
        args: An object with attributes:
            cmake: path to the cmake executable
            ninja: path to the ninja executable
            srcdir: where Shaderc source can be found
            builddir: build working directory
            installdir: install directory
    """

    if not os.path.isdir(args.srcdir):
        raise RuntimeError('Soure directory %s does not exist' % (args.srcdir))

    # Make paths absolute, and ensure directories exist.
    for d in [args.builddir, args.installdir]:
        if not os.path.isdir(d):
            os.makedirs(d)
    cmake = (os.path.abspath(args.cmake)
             if os.path.isfile(args.cmake) else 'cmake')
    ctest = 'ctest' if cmake == 'cmake' else os.path.join(
        os.path.dirname(cmake), 'ctest')
    ninja = (os.path.abspath(args.ninja)
             if os.path.isfile(args.ninja) else 'ninja')
    args.srcdir = os.path.abspath(args.srcdir)
    args.builddir = os.path.abspath(args.builddir)
    args.installdir = os.path.abspath(args.installdir)

    print('Building Shaderc:')
    print('   Source     : ', args.srcdir)
    print('   Cmake      : ', cmake)
    print('   Ninja      : ', ninja)
    print('   Build dir  : ', args.builddir)
    print('   Install dir: ', args.installdir)
    cmake_command = [cmake, args.srcdir, '-GNinja',
                     '-DCMAKE_BUILD_TYPE=%s' % args.buildtype,
                     '-DCMAKE_INSTALL_PREFIX=%s' % (args.installdir)]

    if (OS == 'Windows' or OS.startswith('CYGWIN')):
        bat_content = R'''
call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"
{cmake_batline}
{ninja} install
{ctest}
        '''.strip().format(cmake_batline=quote_some(cmake_command, [0, 1, 4]),
                           ninja=quote(ninja), ctest=quote(ctest))
        bat_filename = os.path.join(args.builddir, 'build.bat')
        open(bat_filename, 'w').write(bat_content)
        run(bat_filename, args.builddir, args.dry_run)
    else:
        run(cmake_command, args.builddir, args.dry_run)
        run([ninja, 'install'], args.builddir, args.dry_run)
        run([ctest], args.builddir, args.dry_run)


def main():
    """Builds Shaderc after parsing argument specifying locations of
    files, level of parallelism, and whether it's a dry run that should
    skip actual compilation and installation."""

    parser = argparse.ArgumentParser(description='Build Shaderc simply')
    parser.add_argument('-n', '--dry_run', dest='dry_run', default=False,
                        action='store_true',
                        help='Dry run: Make dirs and only print commands '
                        ' to be run')
    parser.add_argument('-j', type=int, dest='j', default=4,
                        help='Number of parallel build processes. Default is 4')
    parser.add_argument('--srcdir', dest='srcdir', default='src/shaderc',
                        help='Shaderc source directory. Default "src/shaderc".')
    parser.add_argument('--builddir', dest='builddir', default='out',
                        help='Build directory. Default is "out".')
    parser.add_argument('--installdir', dest='installdir', required=True,
                        help='Installation directory. Required.')
    parser.add_argument('--type', dest='buildtype', default='RelWithDebInfo',
                        help='Build type. Default is RelWithDebInfo')

    arch = None
    if (OS == 'Windows'):
        arch = 'windows-x86'
    if (OS.startswith('CYGWIN')):
        arch = 'windows-x86'
    if (OS == 'Linux'):
        arch = 'linux-x86'
    if (OS == 'Darwin'):
        arch = 'darwin-x86'
    if (arch is None):
        raise RuntimeError('Unknown OS: %s' % OS)

    cmake_default = os.path.join('prebuilts', 'cmake', arch, 'bin', 'cmake')
    parser.add_argument('--cmake', dest='cmake',
                        default=os.path.join(os.getcwd(), cmake_default),
                        help='Path to the cmake executable. Default is %s.'
                        % cmake_default)

    ninja_default = os.path.join('prebuilts', 'ninja', arch, 'ninja')
    parser.add_argument('--ninja', dest='ninja',
                        default=os.path.join(os.getcwd(), ninja_default),
                        help='Path to the ninja executable. Default is %s.'
                        % ninja_default)

    args = parser.parse_args()

    build(args)


if __name__ == '__main__':
    main()
