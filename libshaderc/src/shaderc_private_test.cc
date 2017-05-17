// Copyright 2017 The Shaderc Authors. All rights reserved.
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

#include <gmock/gmock.h>
#include "shaderc_private.h"

namespace {

TEST(ConvertSpecificStage, Exhaustive) {
  EXPECT_EQ(shaderc_util::Compiler::Stage::Vertex,
            shaderc_convert_specific_stage(shaderc_vertex_shader));
  EXPECT_EQ(shaderc_util::Compiler::Stage::Fragment,
            shaderc_convert_specific_stage(shaderc_fragment_shader));
  EXPECT_EQ(shaderc_util::Compiler::Stage::TessControl,
            shaderc_convert_specific_stage(shaderc_tess_control_shader));
  EXPECT_EQ(shaderc_util::Compiler::Stage::TessEval,
            shaderc_convert_specific_stage(shaderc_tess_evaluation_shader));
  EXPECT_EQ(shaderc_util::Compiler::Stage::Geometry,
            shaderc_convert_specific_stage(shaderc_geometry_shader));
  EXPECT_EQ(shaderc_util::Compiler::Stage::Compute,
            shaderc_convert_specific_stage(shaderc_compute_shader));
}
}  // anonymous namespace
