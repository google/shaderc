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

#ifndef GLSLC_SHADER_STAGE_H_
#define GLSLC_SHADER_STAGE_H_

#include <ostream>

#include "glslang/Public/ShaderLang.h"

#include "libshaderc_util/string_piece.h"

namespace glslc {

// A callable class that deduces a shader stage from a file name.
class StageDeducer {
 public:
  StageDeducer(shaderc_util::string_piece file_name) : file_name_(file_name) {}
  // Returns a glslang EShLanguage from the file_name_ member or EShLangCount
  // if the shader stage could not be determined.
  // Any errors are written to error_stream.
  EShLanguage operator()(std::ostream* error_stream,
                         const shaderc_util::string_piece& error_tag) const;

 private:
  shaderc_util::string_piece file_name_;
};

EShLanguage GetShaderStageFromCmdLine(
    const shaderc_util::string_piece& f_shader_stage);

}  // namespace glslc

#endif  // GLSLC_SHADER_STAGE_H_
