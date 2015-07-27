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

#ifndef LIBSHADERC_UTIL_INCLUDER_H
#define LIBSHADERC_UTIL_INCLUDER_H

#include <atomic>

#include "glslang/Public/ShaderLang.h"

namespace shaderc_util {

// An Includer that counts how many #include directives it saw.
class CountingIncluder : public glslang::TShader::Includer {
 public:
  CountingIncluder() : num_include_directives_(0) {}

  // Bumps num_include_directives and returns the results of
  // include_delegate(filename).  Subclasses should override include_delegate()
  // instead of this one.  Also see the base-class version.
  std::pair<std::string, std::string> include(
      const char* filename) const final {
    ++num_include_directives_;
    return include_delegate(filename);
  }

  int num_include_directives() const { return num_include_directives_.load(); }

 private:
  // Invoked by this class to provide results to
  // TShader::Includer::include(filename).
  virtual std::pair<std::string, std::string> include_delegate(
      const char* filename) const = 0;

  // The number of #include directive encountered.
  mutable std::atomic_int num_include_directives_;
};
}

#endif  // LIBSHADERC_UTIL_INCLUDER_H
