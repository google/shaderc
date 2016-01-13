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

#include "file_includer.h"

#include "libshaderc_util/io.h"

namespace glslc {

shaderc_includer_response* FileIncluder::GetInclude(const char* filename) {
  file_full_path_ = file_finder_.FindReadableFilepath(filename);
  if (!file_full_path_.empty() &&
      shaderc_util::ReadFile(file_full_path_, &file_content_)) {
    response_ = {
        file_full_path_.c_str(), file_full_path_.length(), file_content_.data(),
        file_content_.size(),
    };
  } else {
    char error_msg[] = "include file not found.";
    file_content_.insert(file_content_.begin(), error_msg,
                         error_msg + sizeof(error_msg) / sizeof(error_msg[0]));
    response_ = {
        filename, std::strlen(filename), file_content_.data(),
        file_content_.size(),
    };
  }
  return &response_;
}

void FileIncluder::ReleaseInclude(shaderc_includer_response* data) {
  // In the first implementation, we don't need to use data, as we cache only
  // one latest response data. All we need to do is resetting the cached
  // response data to null.
  (void)data;

  response_ = {
      nullptr, 0u, nullptr, 0u,
  };
  file_content_.clear();
  file_full_path_.clear();
}

}  // namespace glslc
