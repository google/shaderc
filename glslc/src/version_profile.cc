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

#include "version_profile.h"

#include <cctype>

namespace {

const int kVersionNumberLength = 3;
const int kMaxProfileLength = 13;  // strlen(compatibility)
const int kMaxVersionProfileLength = kVersionNumberLength + kMaxProfileLength;
const int kMinVersionProfileLength = kVersionNumberLength;

}  // anonymous namespace

namespace glslc {

bool GetVersionProfileFromCmdLine(const std::string& version_profile,
                                  int* version, EProfile* profile) {
  if (version_profile.size() < kMinVersionProfileLength ||
      version_profile.size() > kMaxVersionProfileLength ||
      !::isdigit(version_profile.front()))
    return false;

  size_t split_point;
  int version_number = std::stoi(version_profile, &split_point);
  if (!IsKnownVersion(version_number)) return false;
  *version = version_number;

  const std::string profile_string = version_profile.substr(split_point);
  if (profile_string.empty()) {
    *profile = ENoProfile;
  } else if (profile_string == "core") {
    *profile = ECoreProfile;
  } else if (profile_string == "es") {
    *profile = EEsProfile;
  } else if (profile_string == "compatibility") {
    *profile = ECompatibilityProfile;
  } else {
    return false;
  }

  return true;
}

}  // namespace glslc
