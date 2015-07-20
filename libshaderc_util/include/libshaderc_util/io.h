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

#ifndef LIBSHADERC_UTIL_IO_H_
#define LIBSHADERC_UTIL_IO_H_

#include <string>
#include <vector>

#include "string_piece.h"

namespace shaderc_util {

// Reads all of the characters in a given file into input_data.  Outputs an
// error message to std::cerr if the file could not be read and returns false if
// there was an error.  If the input_file is "-", then input is read from
// std::cin.
bool ReadFile(const std::string& input_file_name,
              std::vector<char>* input_data);

// Returns and initializes the file_stream parameter if the output_filename
// refers to a file, or returns &std::cout if the output_filename is "-".
// Returns nullptr if the file could not be opened for writing.
// If the output refers to a file, and the open failed for writing, file_stream
// is left with its fail_bit set.
std::ostream* GetOutputStream(const string_piece& output_filename,
                              std::ofstream* file_stream);

// Writes output_data to a file, overwriting if it exists.  If output_file_name
// is "-", writes to std::cout.
bool WriteFile(std::ostream* output_stream, const string_piece& output_data);

}  // namespace shaderc_util

#endif  // LIBSHADERC_UTIL_IO_H_
