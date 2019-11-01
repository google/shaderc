// Copyright 2019 The Shaderc Authors. All rights reserved.
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

#include <algorithm>
#include <cstdarg>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "gmock/gmock.h"
#include "spvcir_pass.h"
#include "test/opt/assembly_builder.h"
#include "test/opt/pass_fixture.h"
#include "test/opt/pass_utils.h"

namespace spvtools {
namespace opt {
namespace {

using ::testing::HasSubstr;
using ::testing::MatchesRegex;
using SpvcIrParsingTest = PassTest<::testing::Test>;

std::string SelectiveJoin(const std::vector<const char*>& strings,
                          const std::function<bool(const char*)>& skip_dictator,
                          char delimiter = '\n') {
  std::ostringstream oss;
  for (const auto* str : strings) {
    if (!skip_dictator(str)) oss << str << delimiter;
  }
  return oss.str();
}

std::string JoinAllInsts(const std::vector<const char*>& insts) {
  return SelectiveJoin(insts, [](const char*) { return false; });
}

TEST_F(SpvcIrParsingTest, OpCapabilityInstruction) {
  const std::vector<const char*> input = {
      "OpCapability Shader",
      "OpCapability VulkanMemoryModelKHR",
      "OpExtension \"SPV_KHR_vulkan_memory_model\"",
      "OpMemoryModel Logical VulkanKHR",
      "OpEntryPoint Vertex %func \"shader\"",
      "%void   = OpTypeVoid",
      "%main = OpTypeFunction %void",
      "%func   = OpFunction %void None %main",
      "%label  = OpLabel",
      "OpReturn",
      "OpFunctionEnd"
  };

  std::vector<uint32_t> binary;
  std::unique_ptr<IRContext> context =
      BuildModule(SPV_ENV_UNIVERSAL_1_1, nullptr, JoinAllInsts(input),
                  SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  ASSERT_NE(context, nullptr);
  context->module()->ToBinary(&binary, false);

  spirv_cross::ParsedIR ir;
  ir.spirv =
      std::vector<uint32_t>(binary.data(), binary.data() + binary.size());

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      JoinAllInsts(input), /* skip_nop = */ true, /* do_validation = */ false,
      &ir);

  EXPECT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result));
  EXPECT_EQ(ir.declared_capabilities[0], spv::Capability::CapabilityShader);
  EXPECT_EQ(ir.declared_capabilities[1],
            spv::Capability::CapabilityVulkanMemoryModelKHR);
}

}  // namespace
}  // namespace opt
}  // namespace spvtools
