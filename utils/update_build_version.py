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

# Updates build-version.inc in the current directory, unless the update is
# identical to the existing content.
#
# Args: <shaderc_dir> <spirv-tools_dir> <glslang_dir>
#
# For each directory, there will be a line in build-version.inc containing that
# directory's "git describe" output enclosed in double quotes and appropriately
# escaped.

import datetime
import os.path
import subprocess
import sys

OUTFILE = 'build-version.inc'

def describe(dir):
    """Runs 'git describe' in dir.  If successful, returns the output; otherwise,
returns 'unknown hash, <date>'."""
    try:
        return subprocess.check_output(["git", "describe"], cwd=dir).rstrip()
    except subprocess.CalledProcessError:
        return 'unknown hash, ' + datetime.date.today().isoformat()

def main():
    if len(sys.argv) != 4:
        print 'usage: {0} <shaderc_dir> <spirv-tools_dir> <glslang_dir>'.format(sys.argv[0])
        sys.exit(1)

    new_content = ('"shaderc ' + describe(sys.argv[1]).replace('"', '\\"') + '\\n"\n' +
                   '"spirv-tools ' + describe(sys.argv[2]).replace('"', '\\"') + '\\n"\n' +
                   '"glslang ' + describe(sys.argv[3]).replace('"', '\\"') + '\\n"\n')
    if os.path.isfile(OUTFILE) and new_content == open(OUTFILE, 'r').read():
        sys.exit(0)
    open(OUTFILE, 'w').write(new_content)

if __name__ == '__main__':
    main()
