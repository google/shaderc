// Copyright 2018 The Shaderc Authors. All rights reserved.
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

#ifndef LIBSHADERC_SPVC_SRC_SPVC_PRIVATE_H_
#define LIBSHADERC_SPVC_SRC_SPVC_PRIVATE_H_

#include <string>

#include "shaderc/spvc.h"
#include "spirv_glsl.hpp"

struct spvc_compiler {
};

// Described in spvc.h.
struct spvc_compilation_result {
  virtual ~spvc_compilation_result() {}

  // Output of compilation.
  std::string output;
  // The size of the output data in term of bytes.
  size_t output_data_size = 0;
  // Compilation messages.
  std::string messages;
  // Number of errors.
  size_t num_errors = 0;
  // Number of warnings.
  size_t num_warnings = 0;
  // Compilation status.
  spvc_compilation_status compilation_status =
      spvc_compilation_status_null_result_object;
};

#endif  // LIBSHADERC_SPVC_SRC_SPVC_PRIVATE_H_
