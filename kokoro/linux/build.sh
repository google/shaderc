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
# Linux Build Script.


# Fail on any error.
set -e
# Display commands being run.
set -x

BUILD_ROOT=$PWD
SRC=$PWD/github/shaderc
CONFIG=$1
COMPILER=$2

SKIP_TESTS="False"
BUILD_TYPE="Debug"

CMAKE_C_CXX_COMPILER=""
if [ $COMPILER = "clang" ]
then
  sudo ln -s /usr/bin/clang-3.8 /usr/bin/clang
  sudo ln -s /usr/bin/clang++-3.8 /usr/bin/clang++
  CMAKE_C_CXX_COMPILER="-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++"
fi

# Possible configurations are:
# ASAN, COVERAGE, RELEASE, DEBUG, DEBUG_EXCEPTION, RELEASE_MINGW

if [ $CONFIG = "RELEASE" ] || [ $CONFIG = "RELEASE_MINGW" ]
then
  BUILD_TYPE="RelWithDebInfo"
fi

ADDITIONAL_CMAKE_FLAGS=""
if [ $CONFIG = "ASAN" ]
then
  ADDITIONAL_CMAKE_FLAGS="-DCMAKE_CXX_FLAGS=-fsanitize=address -DCMAKE_C_FLAGS=-fsanitize=address"
  export ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer-3.4
elif [ $CONFIG = "COVERAGE" ]
then
  ADDITIONAL_CMAKE_FLAGS="-DENABLE_CODE_COVERAGE=ON"
  SKIP_TESTS="True"
elif [ $CONFIG = "DEBUG_EXCEPTION" ]
then
  ADDITIONAL_CMAKE_FLAGS="-DDISABLE_EXCEPTIONS=ON -DDISABLE_RTTI=ON"
elif [ $CONFIG = "RELEASE_MINGW" ]
then
  ADDITIONAL_CMAKE_FLAGS="-Dgtest_disable_pthreads=ON -DCMAKE_TOOLCHAIN_FILE=$SRC/cmake/linux-mingw-toolchain.cmake"
  SKIP_TESTS="True"
fi

# Get NINJA.
wget -q https://github.com/ninja-build/ninja/releases/download/v1.7.2/ninja-linux.zip
unzip -q ninja-linux.zip
export PATH="$PWD:$PATH"

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
cmake -GNinja -DRE2_BUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=$BUILD_TYPE $ADDITIONAL_CMAKE_FLAGS $CMAKE_C_CXX_COMPILER ..

echo $(date): Build glslang...
ninja glslangValidator

echo $(date): Build everything...
ninja
echo $(date): Build completed.

echo $(date): Check Shaderc for copyright notices...
ninja check-copyright

if [ $CONFIG = "COVERAGE" ]
then
  echo $(date): Check coverage...
  ninja report-coverage
  echo $(date): Check coverage completed.
fi

echo $(date): Starting ctest...
if [ $SKIP_TESTS = "False" ]
then
  ctest --output-on-failure -j4
fi
echo $(date): ctest completed.

