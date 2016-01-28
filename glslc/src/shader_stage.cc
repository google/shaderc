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

#include "libshaderc_util/resources.h"

#include "file.h"

using shaderc_util::string_piece;

namespace {

// Maps an identifier to a shader stage.
struct StageMapping {
  const char* id;
  shaderc_shader_stage stage;
};

}  // anonymous namespace

namespace glslc {

shaderc_shader_stage MapStageNameToForcedShaderStage(const string_piece& stage_name) {
  const StageMapping string_to_stage[] = {
      {"vertex", shaderc_glsl_vertex_shader},
      {"fragment", shaderc_glsl_fragment_shader},
      {"tesscontrol", shaderc_glsl_tess_control_shader},
      {"tesseval", shaderc_glsl_tess_evaluation_shader},
      {"geometry", shaderc_glsl_geometry_shader},
      {"compute", shaderc_glsl_compute_shader}};
  for (const auto& entry : string_to_stage) {
    if (stage_name == entry.id) return entry.stage;
  }
  return shaderc_glsl_infer_from_source;
}

shaderc_shader_stage GetForcedShaderStageFromCmdLine(
    const shaderc_util::string_piece& f_shader_stage_str) {
  size_t equal_pos = f_shader_stage_str.find_first_of("=");
  if (equal_pos == std::string::npos) return shaderc_glsl_infer_from_source;
  return MapStageNameToForcedShaderStage(f_shader_stage_str.substr(equal_pos + 1));
}

shaderc_shader_stage DeduceDefaultShaderStageFromFileName(
    const string_piece file_name) {
  // Add new stage types here.
  static const StageMapping kStringToStage[] = {
      {"vert", shaderc_glsl_default_vertex_shader},
      {"frag", shaderc_glsl_default_fragment_shader},
      {"tesc", shaderc_glsl_default_tess_control_shader},
      {"tese", shaderc_glsl_default_tess_evaluation_shader},
      {"geom", shaderc_glsl_default_geometry_shader},
      {"comp", shaderc_glsl_default_compute_shader}};

  const string_piece extension = glslc::GetFileExtension(file_name);
  shaderc_shader_stage stage = shaderc_glsl_infer_from_source;

  for (const auto& entry : kStringToStage) {
    if (extension == entry.id) stage = entry.stage;
  }

  return stage;
}

}  // namespace glslc
