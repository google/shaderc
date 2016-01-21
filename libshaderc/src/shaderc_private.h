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

#ifndef LIBSHADERC_SRC_SHADERC_PRIVATE_H_
#define LIBSHADERC_SRC_SHADERC_PRIVATE_H_

#include "shaderc.h"

#include <cstdint>
#include <string>
#include <vector>

// Described in shaderc.h.
struct shaderc_spv_module {
  // Whether compilation succeeded.
  bool compilation_succeeded;
  // SPIR-V binary.
  std::string spirv;
  // Compilation messages.
  std::string messages;
  // Number of errors.
  size_t num_errors = 0;
  // Number of warnings.
  size_t num_warnings = 0;
  // Compilation result.
  shaderc_compilation_result compilation_result;
};

struct shaderc_compiler {
  // Whether or not this compiler is in an initialized state.
  bool initialized = true;
};

#endif  // LIBSHADERC_SRC_SHADERC_PRIVATE_H_
