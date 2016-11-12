// Copyright 2016 The Shaderc Authors. All rights reserved.
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

#include "resource_parse.h"

#include <cstring>
#include <iterator>
#include <sstream>
#include <vector>

namespace {

// Converts a limit string to a limit enum.  Returns true on successful
// conversion.
bool StringToLimit(const std::string& str, shaderc_limit* limit) {
  const char* cstr = str.c_str();
#define RESOURCE(NAME, FIELD, ENUM)    \
  if (0 == std::strcmp(#NAME, cstr)) { \
    *limit = shaderc_limit_##ENUM;     \
    return true;                       \
  }
#include "libshaderc_util/resources.inc"
#undef RESOURCE
  return false;
}
}  //  anonymous namespace

namespace glslc {

bool ParseResourceSettings(const std::string& input,
                           std::vector<ResourceSetting>* limits,
                           std::string* err) {
  auto failure = [err, limits](std::string msg) {
    *err = msg;
    limits->clear();
    return false;
  };
  std::istringstream input_stream(input);
  std::istream_iterator<std::string> pos((input_stream));
  limits->clear();

  while (pos != std::istream_iterator<std::string>()) {
    const std::string limit_name = *pos++;
    shaderc_limit limit = static_cast<shaderc_limit>(0);
    if (!StringToLimit(limit_name, &limit))
      return failure(std::string("Invalid resource limit: " + limit_name));

    if (pos == std::istream_iterator<std::string>())
      return failure(std::string("Missing value after limit: ") + limit_name);

    const std::string value_str = *pos;
    int value;
    std::istringstream value_stream(value_str);
    value_stream >> value;
    if (value_stream.bad() || !value_stream.eof() || value_stream.fail())
      return failure(std::string("Invalid integer: ") + value_str);

    limits->push_back({limit, value});
    ++pos;
  }

  return true;
}
}
