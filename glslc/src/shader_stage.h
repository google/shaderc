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

#include <string>
#include <utility>
#include <vector>

#include "glslang/Public/ShaderLang.h"

#include "libshaderc_util/string_piece.h"

namespace glslc {

// Returns a glslang EShLanguage from a given command line -fshader-stage=
// argument option or EShLangCount if the shader stage could not be determined.
EShLanguage GetShaderStageFromCmdLine(
    const shaderc_util::string_piece& f_shader_stage);

// Returns a glslang EShLanguage from a given filename or EShLangCount
// if the shader stage could not be determined.
EShLanguage GetShaderStageFromFileName(
    const shaderc_util::string_piece& filename);

// Given a string representing a stage, returns the glslang EShLanguage for it.
// If the stage string is not recognized, returns EShLangCount.
EShLanguage MapStageNameToLanguage(
    const shaderc_util::string_piece& stage_name);

}  // namespace glslc

#endif  // GLSLC_SHADER_STAGE_H_
