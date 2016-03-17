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
import sys


OS = platform.system()


def run(cmd, cwd, env, justprint):
    """Prints a command to run, and optionally runs it.

    Raises a RuntimeError if the command does not launch or otherwise fails.

    Args:
      justprint: If true, then only print the command. Otherwise run the 
                   command after printing it.
      cmd:       List of words in the command.
      cwd:       Working directory for the command.
      env:       Environment to pass to subprocess.
    """
    print(cmd)
    if justprint:
        return

    p = subprocess.Popen(cmd, cwd=cwd, env=env)
    (_, _) = p.communicate()
    if p.returncode != 0:
        raise RuntimeError('Failed to run %s in %s' % (cmd, cwd))


def build(args):
    """ Builds Shaderc under specified conditions.

    Args:
        args: An object with attributes:
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
    args.srcdir = os.path.abspath(args.srcdir)
    args.builddir = os.path.abspath(args.builddir)
    args.installdir = os.path.abspath(args.installdir)

    print('Building Shaderc:')
    print('   Source     : ', args.srcdir)
    print('   Build dir  : ', args.builddir)
    print('   Install dir: ', args.installdir)
    cmake_command = ['cmake', args.srcdir, '-GNinja',
                     '-DCMAKE_BUILD_TYPE=%s' % args.buildtype,
                     '-DCMAKE_INSTALL_PREFIX=%s' % args.installdir]

    env = None
    if OS == 'Windows':
        p = subprocess.Popen(
            '"%VS140COMNTOOLS%..\\..\\VC\\vcvarsall.bat" & set',
            stdout=subprocess.PIPE, cwd=args.builddir, shell=True)
        env = dict([tuple(line.split('=', 1))
                    for line in p.communicate()[0].splitlines()])
    run(cmake_command, args.builddir, env, args.dry_run)
    run(['ninja', 'install'], args.builddir, env, args.dry_run)
    run(['ctest', '--output-on-failure'], args.builddir, env, args.dry_run)


def main():
    """Builds Shaderc after parsing argument specifying locations of
    files, level of parallelism, and whether it's a dry run that should
    skip actual compilation and installation."""

    parser = argparse.ArgumentParser(description='Build Shaderc simply')
    parser.add_argument('-n', '--dry_run', dest='dry_run', default=False,
                        action='store_true',
                        help='Dry run: Make dirs and only print commands '
                        ' to be run')
    parser.add_argument('--srcdir', dest='srcdir', default='src/shaderc',
                        help='Shaderc source directory. Default "src/shaderc".')
    parser.add_argument('--builddir', dest='builddir', default='out',
                        help='Build directory. Default is "out".')
    parser.add_argument('--installdir', dest='installdir', required=True,
                        help='Installation directory. Required.')
    parser.add_argument('--type', dest='buildtype', default='RelWithDebInfo',
                        help='Build type. Default is RelWithDebInfo')

    arch = None
    if (OS == 'Windows' or OS.startswith('CYGWIN')):
        arch = 'windows-x86'
    if OS == 'Linux':
        arch = 'linux-x86'
    if OS == 'Darwin':
        arch = 'darwin-x86'
    if arch is None:
        raise RuntimeError('Unknown OS: %s' % OS)

    path_default = (os.path.join(os.getcwd(), 'prebuilts', 'cmake', arch, 'bin')
                    + os.pathsep +
                    os.path.join(os.getcwd(), 'prebuilts', 'ninja', arch)
                    + os.pathsep +
                    os.path.join(os.getcwd(), 'prebuilts', 'python', arch,
                                 'x64'))
    parser.add_argument('--path', dest='path',
                        default=path_default,
                        help='Extra directories to prepend to the system path, '
                        'separated by your system\'s path delimiter (typically '
                        '":" or ";"). After prepending, path must contain '
                        'cmake, ninja, and python. On Cygwin, the native '
                        'Windows Python must come first. Default is %s.'
                        % path_default)

    args = parser.parse_args()

    if args.path:
        os.putenv('PATH', args.path + os.pathsep + os.getenv('PATH'))

    if OS.startswith('CYGWIN'):
        os.execlp('python', 'python', *sys.argv) # Escape to Windows.

    build(args)


if __name__ == '__main__':
    main()
