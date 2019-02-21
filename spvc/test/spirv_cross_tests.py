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

import filecmp
import os.path
import subprocess
import sys
import tempfile

spvc_path = sys.argv[1]
cases_path = sys.argv[2]
expect_dir = sys.argv[3]
cases_dir = os.path.dirname(cases_path)

not_used, tmpfile = tempfile.mkstemp()

test_count = 0
run_count = 0
pass_count = 0

cases_file = open(cases_path, 'r')
for line in cases_file:
    line = line[:-1]
    if not line or line[:1] == "#":
        continue

    test_count += 1
    if line.startswith('skip '):
        continue
    run_count += 1

    split = line.split()
    input_spv = os.path.join(cases_dir, split[0])
    expect_path = os.path.join(expect_dir, split[1])
    flags = split[2:]
    command = [spvc_path, '-o', tmpfile, '--validate=vulkan1.1' ] + flags + [input_spv]
    subprocess.check_call(command)

    if filecmp.cmp(tmpfile, expect_path):
        pass_count += 1
    else:
        print 'FAIL', line

cases_file.close()
os.remove(tmpfile)

print test_count, 'test cases'
print run_count, 'not skipped'
print pass_count, 'passed'

if pass_count != run_count:
    sys.exit(1)
