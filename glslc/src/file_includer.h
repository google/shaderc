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

#include "glslang/Public/ShaderLang.h"

#include "libshaderc_util/counting_includer.h"
#include "libshaderc_util/file_finder.h"

namespace glslc {

// A CountingIncluder for files.  Translates the #include argument into the
// file's contents, based on a FileFinder.
class FileIncluder : public shaderc_util::CountingIncluder {
 public:
  FileIncluder(const shaderc_util::FileFinder* file_finder)
      : file_finder_(*file_finder) {}

 private:
  // Finds filename in search path and returns its contents.
  std::pair<std::string, std::string> include_delegate(
      const char* filename) const override;

  // Used by include() to get the full filepath.
  const shaderc_util::FileFinder& file_finder_;
};

}  // namespace glslc

#endif  // GLSLC_FILE_INCLUDER_H_
