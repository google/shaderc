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

#include "shader_stage.h"

#include "libshaderc_util/shader_stage.h"
#include "libshaderc_util/file_finder.h"
#include "libshaderc_util/resources.h"

#include "file.h"
#include "file_includer.h"

using shaderc_util::string_piece;

namespace {

// Maps an identifier to a language.
struct LanguageMapping {
  const char* id;
  EShLanguage language;
};

}  // anonymous namespace

namespace glslc {

EShLanguage StageDeducer::operator()(std::ostream* error_stream,
                                     const string_piece& error_tag) const {
  // Add new stage types here.
  static const LanguageMapping kStringToStage[] = {
      {"vert", EShLangVertex},      {"frag", EShLangFragment},
      {"tesc", EShLangTessControl}, {"tese", EShLangTessEvaluation},
      {"geom", EShLangGeometry},    {"comp", EShLangCompute}};

  const string_piece extension = glslc::GetFileExtension(file_name_);
  EShLanguage stage = EShLangCount;

  for (const auto& entry : kStringToStage) {
    if (extension == entry.id) stage = entry.language;
  }

  if (stage == EShLangCount) {
    *error_stream << "glslc: error: '" << file_name_ << "': ";
    if (IsGlslFile(file_name_)) {
      *error_stream << ".glsl file encountered but no -fshader-stage "
                       "specified ahead";
    } else if (file_name_ == "-") {
      *error_stream << "-fshader-stage required when input is from standard "
                       "input \"-\"";
    } else {
      *error_stream << "file not recognized: File format not recognized";
    }
    *error_stream << "\n";
  }
  return stage;
}

EShLanguage GetShaderStageFromCmdLine(
    const shaderc_util::string_piece& f_shader_stage) {
  size_t equal_pos = f_shader_stage.find_first_of("=");
  if (equal_pos == std::string::npos) return EShLangCount;
  return MapStageNameToLanguage(f_shader_stage.substr(equal_pos + 1));
}
}  // namespace glslc
