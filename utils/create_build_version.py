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

# Generates build_version.h in the current directory.
#
# Args: <shaderc_dir> <spirv-tools_dir> <glslang_dir>
#
# For each directory, there will be a line in build_version.h containing that
# directory's "git describe" output enclosed in double quotes and appropriately
# escaped.

import subprocess
import sys

def describe(dir):
    """Runs 'git describe' in dir.  If successful, returns the output; otherwise,
returns 'unknown: git-describe error'."""
    try:
        return subprocess.check_output(["git", "describe"], cwd=dir).rstrip()
    except subprocess.CalledProcessError:
        return 'unknown: git-describe error'

def main():
    if (len(sys.argv) != 4):
        print 'usage: {0} <shaderc_dir> <spirv-tools_dir> <glslang_dir>'.format(sys.argv[0])
        sys.exit(1)
    version_file = open('build-version.h', 'w')
    version_file.write('"shaderc ' + describe(sys.argv[1]).replace('"', '\\"') + '\\n"\n')
    version_file.write('"spirv-tools ' + describe(sys.argv[2]).replace('"', '\\"') + '\\n"\n')
    version_file.write('"glslang ' + describe(sys.argv[3]).replace('"', '\\"') + '\\n"\n')
    version_file.close()

if __name__ == '__main__':
    main()
