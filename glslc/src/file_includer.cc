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

std::pair<std::string, std::string> FileIncluder::include(
    const char* filename) const {
  ++num_include_directives_;

  const std::string full_path = file_finder_.FindReadableFilepath(filename);
  std::vector<char> content;
  if (!full_path.empty() && shaderc_util::ReadFile(full_path, &content)) {
    return std::make_pair(full_path,
                          std::string(content.begin(), content.end()));
  } else {
    // TODO(deki): Emit error message.
    return std::make_pair("", "");
  }
}

}  // namespace glslc
