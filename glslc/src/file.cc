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

#include "file.h"

namespace glslc {

shaderc_util::string_piece GetFileExtension(
    const shaderc_util::string_piece& filename) {
  size_t dot_pos = filename.find_last_of(".");
  if (dot_pos == shaderc_util::string_piece::npos) return "";
  return filename.substr(dot_pos + 1);
}

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(_WIN32)
#define strdup _strdup
#define isseparator(c) ((c) == '/' || (c) == '\\')
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/stat.h>
#include <string.h>
#define isseparator(c) ((c) == '/')
#endif


bool CreateIntermediateDirectories(const std::string &filename)
{
    char *path = strdup(filename.c_str());
    int len = filename.length();

    /* Find the file. Remove it from the path. */
    for (int i = len - 1; i >= 0; --i) {
        if (isseparator(path[i])) {
            path[i] = 0;
            break;
        }
    }

    /* Create all intermediate directories. */
    for (int i = 0; i < len; ++i) {
        if (isseparator(path[i])) {
            char c = path[i];
            path[i] = 0;
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(_WIN32)
            CreateDirectoryA(path, nullptr);
#else
            mkdir(path, 0755);
#endif
            path[i] = c;
        }
    }

    int ret = mkdir(path, 0755);
    free(path);
    return ret == 0;
}

}  // namespace glslc
