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

'''
Go through all the spirv-cross test shaders, convert each to spir-v
binary and store that in the shaderc spvc test directory.
Produce a list of all the tests that includes the flags to be used.
'''

import errno
import os
import subprocess
import sys

# location of spirv-cross test shader sources, relative to this directory
spirv_cross_dir = os.path.normpath('../../third_party/spirv-cross')

# where the corresponding binaries live in shaderc, relative to this directory
shaderc_spirv_cross_test_inputs = 'spirv_cross_test_inputs'

test_case_dirs = (
# directory           language  optimize
('shaders'            , 'glsl', ''   ),
('shaders'            , 'glsl', 'opt'),
('shaders-no-opt'     , 'glsl', ''   ),
('shaders-msl'        , 'msl' , ''   ),
('shaders-msl'        , 'msl' , 'opt'),
('shaders-msl-no-opt' , 'msl' , ''   ),
('shaders-hlsl'       , 'hlsl', ''   ),
('shaders-hlsl'       , 'hlsl', 'opt'),
('shaders-hlsl-no-opt', 'hlsl', ''   ),
('shaders-reflection' , 'refl', ''   ),
)

spirv_as_path = os.path.join(spirv_cross_dir, 'external', 'spirv-tools-build',
                             'output', 'bin', 'spirv-as')
glslangValidator_path = os.path.join(spirv_cross_dir, 'external',
                                     'glslang-build', 'output', 'bin',
                                     'glslangValidator')

def mkdir_p(filepath):
    ''' make sure directory exists to contain given file '''
    dirpath = os.path.dirname(filepath)
    try:
        os.makedirs(dirpath)
    except OSError as exc:
        if not (exc.errno == errno.EEXIST and os.path.isdir(dirpath)):
            raise

devnull = open(os.devnull, 'w')

def spirv_as(inp, out):
    subprocess.check_call([spirv_as_path, '-o', out, inp], stdout=devnull)

def glslangValidator(inp, out):
    subprocess.check_call([glslangValidator_path , '--target-env', 'vulkan1.1', '-V', '-o',
                          out, inp], stdout=devnull)

def make_test_case(dirpath, filename, language, optimize):
    test_case_path = os.path.join(dirpath, filename)
    input_source_path = os.path.join(spirv_cross_dir, test_case_path)
    input_binary_path = os.path.join(shaderc_spirv_cross_test_inputs, test_case_path)
    expected_output_path = os.path.join('reference', test_case_path)

    if language == 'glsl':
        # convert text source to binary
        mkdir_p(input_binary_path)
        if '.asm.' in filename:
            # spirv text
            spirv_as(input_source_path, input_binary_path)
        else:
            # glsl
            glslangValidator(input_source_path, input_binary_path)

        # build up compiler flags
        flags = ''
        if optimize:
            flags += ' --opt'
        if not '.noeliminate' in filename:
            flags += ' --remove-unused-variables'
        if '.legacy.' in filename:
            flags += ' --version 100 --es'
        if '.flatten.' in filename:
            flags += ' --flatten-ubo'
        if '.flatten_dim.' in filename:
            flags += ' --flatten-multidimensional-arrays'
        if '.sso.' in filename:
            flags += ' --separate-shader-objects'

        # print test case
        if not '.nocompat.' in filename:
            print test_case_path, expected_output_path, flags
        if '.vk.' in filename:
            print test_case_path, expected_output_path + '.vk', flags + ' --vulkan-semantics'

    else:
        # TODO: other languages
        print 'skip', test_case_path, expected_output_path

for test_case_dir, language, optimize in test_case_dirs:
    top = os.path.join(spirv_cross_dir, test_case_dir)
    for dirpath, dirnames, filenames in os.walk(top):
        dirnames.sort()
        for filename in sorted(filenames):
            relative_dirpath = os.path.relpath(dirpath, spirv_cross_dir)
            make_test_case(relative_dirpath, filename, language, optimize)
