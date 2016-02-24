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


def command_output(cmd, dir):
    """Runs a command in a directory and returns its standard output stream.

    Captures the standard error stream.

    Raises a RuntimeError if the command fails to launch or otherwise fails.
    """
    p = subprocess.Popen(cmd,
                         cwd=dir,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    (stdout, _) = p.communicate()
    if p.returncode !=0:
        raise RuntimeError("Failed to run %s in %s" % (cmd, dir))
    return stdout


def describe(dir):
    """Returns a string describing the current Git HEAD version as descriptively
    as possible.

    Runs 'git describe', or alternately 'git rev-parse HEAD', in dir.  If
    successful, returns the output; otherwise returns 'unknown hash, <date>'."""
    try:
        return command_output(["git", "describe"], dir).rstrip()
    except:
        try:
            return command_output(["git", "rev-parse", "HEAD"], dir).rstrip()
        except:
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
