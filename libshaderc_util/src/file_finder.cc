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

#include "libshaderc_util/file_finder.h"

#include <cassert>
#include <fstream>
#include <ios>

namespace {

// Returns "" if path is empty or ends in '/'.  Otherwise, returns "/".
std::string MaybeSlash(const std::string& path) {
  return (path.empty() || path.back() == '/') ? "" : "/";
}

}  // anonymous namespace

namespace shaderc_util {

std::string FileFinder::FindReadableFilepath(
    const std::string& filename) const {
  assert(!filename.empty());
  static const auto for_reading = std::ios_base::in;
  std::filebuf opener;
  for (const auto& prefix : search_path_) {
    const std::string prefixed_filename =
        prefix + MaybeSlash(prefix) + filename;
    if (opener.open(prefixed_filename, for_reading)) return prefixed_filename;
  }
  return "";
}

}  // namespace shaderc_util
