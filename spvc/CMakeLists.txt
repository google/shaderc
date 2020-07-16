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

add_executable(spvc_exe src/main.cc)
shaderc_default_compile_options(spvc_exe)
target_include_directories(spvc_exe PRIVATE ${shaderc_SOURCE_DIR}/libshaderc/include ${spirv-tools_SOURCE_DIR}/include)
set_target_properties(spvc_exe PROPERTIES OUTPUT_NAME spvc)
target_link_libraries(spvc_exe PRIVATE shaderc_spvc shaderc_util)
add_dependencies(spvc_exe build-version)

shaderc_add_asciidoc(spvc_doc_README README)

if(SHADERC_ENABLE_INSTALL)
  install(TARGETS spvc_exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(SHADERC_ENABLE_INSTALL)

if(${SHADERC_ENABLE_TESTS})
  add_subdirectory(test)
endif()
