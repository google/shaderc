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
# Android Build Script.


# Fail on any error.
set -e
# Display commands being run.
set -x

BUILD_ROOT=$PWD
SRC=$PWD/github/shaderc
TARGET_ARCH=$1

# Get NINJA.
wget -q https://github.com/ninja-build/ninja/releases/download/v1.7.2/ninja-linux.zip
unzip -q ninja-linux.zip
export PATH="$PWD:$PATH"
export BUILD_NDK=ON
export ANDROID_NDK=/opt/android-ndk-r15c
git clone --depth=1 https://github.com/taka-no-me/android-cmake.git android-cmake
export TOOLCHAIN_PATH=$PWD/android-cmake/android.toolchain.cmake


cd $SRC/third_party
git clone https://github.com/google/googletest.git
git clone https://github.com/google/glslang.git
git clone https://github.com/KhronosGroup/SPIRV-Tools.git spirv-tools
git clone https://github.com/KhronosGroup/SPIRV-Headers.git spirv-headers
git clone https://github.com/google/re2 spirv-tools/external/re2
git clone https://github.com/google/effcee spirv-tools/external/effcee

cd $SRC/
mkdir build
cd $SRC/build

# Invoke the build.
BUILD_SHA=${KOKORO_GITHUB_COMMIT:-$KOKORO_GITHUB_PULL_REQUEST_COMMIT}
echo $(date): Starting build...
cmake -DRE2_BUILD_TESTING=OFF -GNinja -DCMAKE_BUILD_TYPE=Release -DANDROID_NATIVE_API_LEVEL=android-14 -DANDROID_ABI=$TARGET_ARCH -DSHADERC_SKIP_TESTS=ON -DSPIRV_SKIP_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_PATH -DANDROID_NDK=$ANDROID_NDK ..


echo $(date): Build glslang...
ninja glslangValidator

echo $(date): Build everything...
ninja

echo $(date): Check Shaderc for copyright notices...
ninja check-copyright

echo $(date): Build completed.
