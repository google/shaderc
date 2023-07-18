#!/bin/bash

# Copyright (C) 2017 Google Inc.
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
#
# MacOS Build Script.

# Fail on any error.
set -e
# Display commands being run.
set -x

BUILD_ROOT=$PWD
SRC=$PWD/github/shaderc
BUILD_TYPE=$1

# Get NINJA.
wget -q https://github.com/ninja-build/ninja/releases/download/v1.7.2/ninja-mac.zip
unzip -q ninja-mac.zip
chmod +x ninja
export PATH="$PWD:$PATH"

cd $SRC
./utils/git-sync-deps

mkdir build
cd $SRC/build

# Invoke the build.
BUILD_SHA=${KOKORO_GITHUB_COMMIT:-$KOKORO_GITHUB_PULL_REQUEST_COMMIT}
echo $(date): Starting build...
cmake -GNinja -DCMAKE_INSTALL_PREFIX=$KOKORO_ARTIFACTS_DIR/install -DRE2_BUILD_TESTING=OFF -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

echo $(date): Build glslang...
ninja glslangValidator

echo $(date): Build everything...
ninja

echo $(date): Check Shaderc for copyright notices...
ninja check-copyright

echo $(date): Build completed.

echo $(date): Starting ctest...
ctest --output-on-failure -j4
echo $(date): ctest completed.

# Package the build.
ninja install
cd $KOKORO_ARTIFACTS_DIR
tar czf install.tgz install
