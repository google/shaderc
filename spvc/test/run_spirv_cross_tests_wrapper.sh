#!/usr/bin/env bash

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

# Wrapper for run_spirv_cross_tests.py that takes in the build directory and
# spirv-cross locations, and assumes they are laid out in a standard way,
# instead of requiring your to specify all of the paths. Assumes that is it a
# sibling to the python file in the directory structure.
#
# To invoke:
#   run_spirv_cross_tests_wrapper.sh <build_dir> <cross_dir>
#
#     <build_dir> is the directory the repo was built in, paths to binaries are
#                 expected to conform to a default layout.
#                 NOTE: If you need to specify per-binary paths, you will need to
#                 invoke the .py directly.
#     <cross_dir> is the directory the external spirv-cross repo was checked out
#                 to.

if [ "$#" -lt 2 ]; then
  echo "At least 2 arguments are required, $# provided"
  exit 1
fi

script_path=$(dirname "$0")
build_path="$1"

spvc_path="$1/spvc/spvc"
glslang_path="$1/third_party/glslang/StandAlone/glslangValidator"
spirv_tools_path="$1/third_party/spirv-tools/tools"
spirv_as_path="$spirv_tools_path/spirv-as"
spirv_opt_path="$spirv_tools_path/spirv-opt"
cross_path="$2"
spvc_test_dir="$script_path"

shift 2

python3 $script_path/run_spirv_cross_tests.py "$spvc_path" "$spirv_as_path" "$spirv_opt_path" "$glslang_path" "$cross_path" "$spvc_test_dir" $@
