# Copyright 2020 The Shaderc Authors. All rights reserved.
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

if(${SHADERC_ENABLE_TESTS})
  add_test(NAME spvc_spirv_cross_tests
    COMMAND ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/run_spirv_cross_tests.py
    $<TARGET_FILE:spvc_exe>
    $<TARGET_FILE:spirv-as>
    $<TARGET_FILE:spirv-opt>
    $<TARGET_FILE:glslangValidator>
    ${SHADERC_SPIRV_CROSS_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR})
 add_test(NAME spvc_spirv_cross_spvc_parser_tests
   COMMAND ${PYTHON_EXECUTABLE}
   ${CMAKE_CURRENT_SOURCE_DIR}/run_spirv_cross_tests.py
   $<TARGET_FILE:spvc_exe>
   $<TARGET_FILE:spirv-as>
   $<TARGET_FILE:spirv-opt>
   $<TARGET_FILE:glslangValidator>
   ${SHADERC_SPIRV_CROSS_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}
   --run_spvc_parser_tests)
endif()
