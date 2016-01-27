// Copyright 2015 The Shaderc Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GLSLC_FILE_INCLUDER_H_
#define GLSLC_FILE_INCLUDER_H_

#include <string>
#include <utility>
#include <vector>

#include "libshaderc_util/file_finder.h"
#include "shaderc.hpp"

namespace glslc {

// An includer for files implementing shaderc's includer interface. It resposnes
// the file including query from the compiler with the full path and content of
// the file to be included. In the case that the file is not found or cannot be
// opened, the full path field of in the response will be an empty string, and
// error message will be passed to the content field.
class FileIncluder : public shaderc::CompileOptions::IncluderInterface {
 public:
  FileIncluder(const shaderc_util::FileFinder* file_finder)
      : file_finder_(*file_finder) {}
  shaderc_includer_response* GetInclude(const char* filename) override;
  void ReleaseInclude(shaderc_includer_response* data) override;

 private:
  // Used by GetInclude() to get the full filepath.
  const shaderc_util::FileFinder& file_finder_;
  shaderc_includer_response response_;
  std::vector<char> file_content_;
  std::string file_full_path_;
};

}  // namespace glslc

#endif  // GLSLC_FILE_INCLUDER_H_
