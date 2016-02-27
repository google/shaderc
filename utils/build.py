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

"""
Builds the Shaderc project, on Linux, Darwin, or Windows.

For windows, we assume we have bash and basic shell commands, and we'll
use the MSVC compiler.

For Linux and Mac, use the default C++ compiler.

We require cmake and ninja commands to be present under the prebuilts
directory in the specified root directory.
"""

import argparse
import os
import os.path
import subprocess
import sys


def run(args, cmd, cwd):
    """Prints a command to run, and optionally runs it.

    Raises a RuntimeError if the command does not launch or otherwise fails.

    Args:
      args: Options.  If args.dry_run is true, then only print the command.
            Otherwise run the command after printing it.
      cmd:  List of words in the command.
      cwd:  Working directory for the command.
    """
    print cmd
    if args.dry_run:
        return

    p = subprocess.Popen(cmd, cwd=cwd)
    (_, _) = p.communicate()
    if p.returncode !=0:
        raise RuntimeError("Failed to run %s in %s" % (cmd, dir))


def build(args):
    """ Builds Shaderc under specified conditions.

    Args:
        args: An object with attributes:
            prebuiltdir: where prebuilts can be found
            srcdir: where Shaderc source can be found
            builddir: build working directory
            installdir: install directory
            platform: assumed value of sys.platform
    """

    if not os.path.isdir(args.srcdir):
        raise RuntimeError("Soure directory %s does not exist" % (args.srcdir))
    if not os.path.isdir(args.prebuiltdir):
        raise RuntimeError("Prebuilt directory %s does not exist" % (args.prebuiltdir))

    # Make paths absolute, and enusre directories exist.
    for d in [args.builddir, args.installdir]:
        if not os.path.isdir(d):
            os.makedirs(d)
    args.prebuiltdir = os.path.abspath(args.prebuiltdir)
    args.srcdir = os.path.abspath(args.srcdir)
    args.builddir = os.path.abspath(args.builddir)
    args.installdir = os.path.abspath(args.installdir)

    OS = None
    if args.platform.startswith('linux'):
        OS='linux-x86'
    if args.platform.startswith('darwin'):
        OS='darwin-x86'
    if args.platform.startswith('win') or args.platform.startswith('cygwin'):
        OS='windows-x86'
    if OS is None:
        raise RuntimeError("Unknown platform: %s" % (args.platform))

    cmake = os.path.join(args.prebuiltdir, 'prebuilts', 'cmake', OS, 'bin', 'cmake')
    ninja = os.path.join(args.prebuiltdir, 'prebuilts', 'ninja', OS, 'ninja')

    print 'Building Shaderc:'
    print '   Source     : ', args.srcdir
    print '   Prebuilt   : ', args.prebuiltdir
    print '   Build dir  : ', args.builddir
    print '   Install dir: ', args.installdir
    cmake_command = [cmake, '-GNinja', '-DCMAKE_BUILD_TYPE=RelWithDebInfo',
                    '-DCMAKE_INSTALL_PREFIX=%s' % (args.installdir),
                     args.srcdir]

    # Force use of MSVC on Windows.
    if OS is 'windows-x86':
        cmake_command.append('-DCMAKE_CXX_COMPILER=cl')

    # Configure the project.
    run(args, cmake_command, args.builddir)
    # Install, after building necessary components.
    run(args, [ninja, 'install'], args.builddir)


def main():
    """Builds Shaderc after parsing argument specifying locations of
    files, level of parallelism, and whether it's a dry run that should
    skip actual compilation and installation."""

    parser = argparse.ArgumentParser(description="Build Shaderc simply")
    parser.add_argument('-n', '--dry_run', dest='dry_run', default=False,
                        action='store_true',
                        help='Dry run: Make dirs and only print commands '
                             ' to be run')
    parser.add_argument('-j', type=int, dest='j', default=4,
                        help='Number of parallel build processes. Default is 4')
    parser.add_argument('--prebuiltdir', dest='prebuiltdir', default=os.getcwd(),
                        help='Directory where we can find prebuilts for'
                             ' cmake and ninja. Default is current dir.')
    parser.add_argument('--srcdir', dest='srcdir', default='src/shaderc',
                        help='Shaderc source directory. Default src/shaderc')
    parser.add_argument('--builddir', dest='builddir', default='out',
                        help='Build directory. Default is out')
    parser.add_argument('--installdir', dest='installdir', required=True,
                        help='Installation directory. Required.')
    parser.add_argument('--type', dest='buildtype', default='RelWithDebInfo',
                        help='Build type. Default is RelWithDebInfo')
    parser.add_argument('--platform', dest='platform', default=sys.platform,
                        help='Assumed platform. Default is current platform.')

    args = parser.parse_args()

    build(args)


if __name__ == '__main__':
    main()
