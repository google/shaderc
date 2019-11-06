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

// For a given result Id returns the previously constructed instruction.
template <typename T>
T& get(uint32_t id, spirv_cross::ParsedIR* ir) {
  return spirv_cross::variant_get<T>(ir->ids[id]);
}

// For a given result Id returns the instruction if it was previously
// constructed and had the same result Type otherwise returns nullptr.
template <typename T>
T* maybe_get(uint32_t id, spirv_cross::ParsedIR* ir) {
  if (id >= ir->ids.size())
    return nullptr;
  else if (ir->ids[id].get_type() == static_cast<spirv_cross::Types>(T::type))
    return &get<T>(id, ir);
  else
    return nullptr;
}

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

void createSpvcIr(spirv_cross::ParsedIR* ir, std::string text) {
  std::vector<uint32_t> binary;
  std::unique_ptr<IRContext> context =
      BuildModule(SPV_ENV_UNIVERSAL_1_1, nullptr, text,
                  SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  assert(context && "context");
  context->module()->ToBinary(&binary, false);
  ir->spirv =
      std::vector<uint32_t>(binary.data(), binary.data() + binary.size());
  return;
}

class SpvcIrParsingTest : public PassTest<::testing::Test> {
 protected:
  void SetUp() override {
    input_ = R"(
              OpCapability Shader
              OpCapability VulkanMemoryModelKHR
              OpExtension "SPV_KHR_vulkan_memory_model"
              OpMemoryModel Logical VulkanKHR
              OpEntryPoint Vertex %1 "shader"
         %2 = OpTypeVoid
         %3 = OpTypeFunction %2
         %1 = OpFunction %2 None %3
         %4 = OpLabel
              OpReturn
              OpFunctionEnd
    )";

    before_ = {"OpCapability Shader", "OpCapability VulkanMemoryModelKHR",
                  "OpExtension \"SPV_KHR_vulkan_memory_model\"",
                  "OpMemoryModel Logical VulkanKHR"};

    after_ = {"OpEntryPoint Vertex %1 \"shader\"",
                   "%2 = OpTypeVoid",
                   "%3 = OpTypeFunction %2",
                   "%1 = OpFunction %2 None %3",
                   "%4 = OpLabel",
                   "OpReturn",
                   "OpFunctionEnd"};

    SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
    std::vector<uint32_t> binary;
    std::unique_ptr<IRContext> context =
        BuildModule(SPV_ENV_UNIVERSAL_1_1, nullptr, input_,
                    SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
    assert(context && "context");
    context->module()->ToBinary(&binary, false);
    ir_.spirv =
        std::vector<uint32_t>(binary.data(), binary.data() + binary.size());
  }
  std::string input_;
  spirv_cross::ParsedIR ir_;
  std::vector<const char*> before_;
  std::vector<const char*> after_;
};

TEST_F(SpvcIrParsingTest, OpCapabilityInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
        << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  ASSERT_EQ(ir_.declared_capabilities.size(), 2);
  EXPECT_EQ(ir_.declared_capabilities[0], spv::Capability::CapabilityShader);
  EXPECT_EQ(ir_.declared_capabilities[1],
            spv::Capability::CapabilityVulkanMemoryModelKHR);
}

TEST_F(SpvcIrParsingTest, OpExtensionInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  ASSERT_EQ(ir_.declared_extensions.size(), 1);
  EXPECT_EQ(ir_.declared_extensions[0], "SPV_KHR_vulkan_memory_model");
}

TEST_F(SpvcIrParsingTest, OpMemoryModelInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  EXPECT_EQ(ir_.addressing_model, spv::AddressingModel::AddressingModelLogical);
  EXPECT_EQ(ir_.memory_model, spv::MemoryModel::MemoryModelVulkanKHR);
}

TEST_F(SpvcIrParsingTest, OpEntryPointInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  ASSERT_EQ(ir_.entry_points.size(), 1);
  const auto functionId = 1;
  const auto entry = *ir_.entry_points.begin();
  EXPECT_EQ(entry.first, static_cast<spirv_cross::FunctionID>(functionId));
  EXPECT_EQ(entry.second.orig_name, "shader");
  EXPECT_EQ(entry.second.model, spv::ExecutionModelVertex);
  EXPECT_EQ(entry.second.self,
            static_cast<spirv_cross::FunctionID>(functionId));
  EXPECT_EQ(ir_.meta[functionId].decoration.alias, "shader");
}

TEST_F(SpvcIrParsingTest, OpTypeVoidInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto func_type = maybe_get<spirv_cross::SPIRType>(2, &ir_);
  ASSERT_NE(func_type, nullptr);
  EXPECT_EQ(func_type->basetype, spirv_cross::SPIRType::Void);
}

TEST_F(SpvcIrParsingTest, OpTypeFunctionInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto func = maybe_get<spirv_cross::SPIRFunctionPrototype>(3, &ir_);
  ASSERT_NE(func, nullptr);
  EXPECT_EQ(func->return_type, static_cast<spirv_cross::TypeID>(2));
  EXPECT_TRUE(func->parameter_types.empty());
}

TEST_F(SpvcIrParsingTest, OpFunctionInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto func = maybe_get<spirv_cross::SPIRFunction>(1, &ir_);
  ASSERT_NE(func, nullptr);
  EXPECT_EQ(func->type, spirv_cross::TypeFunction);
  EXPECT_EQ(func->return_type, static_cast<spirv_cross::TypeID>(2));
  EXPECT_EQ(func->function_type, static_cast<spirv_cross::TypeID>(3));
  EXPECT_TRUE(func->arguments.empty());
}

TEST_F(SpvcIrParsingTest, OpLabelInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto func = maybe_get<spirv_cross::SPIRFunction>(1, &ir_);
  ASSERT_NE(func, nullptr);
  auto block = maybe_get<spirv_cross::SPIRBlock>(4, &ir_);
  ASSERT_NE(block, nullptr);
  EXPECT_EQ(func->blocks.size(), static_cast<size_t>(1));
  EXPECT_EQ(func->blocks.data()[0], static_cast<spirv_cross::TypeID>(4));
  EXPECT_EQ(func->entry_block, static_cast<spirv_cross::TypeID>(4));
}

TEST_F(SpvcIrParsingTest, OpReturnInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto block = maybe_get<spirv_cross::SPIRBlock>(4, &ir_);
  ASSERT_NE(block, nullptr);
  EXPECT_EQ(block->terminator, spirv_cross::SPIRBlock::Return);
}

TEST_F(SpvcIrParsingTest, SpvOpSourceInstruction) {

  const std::vector<const char*> middle = {"OpSource HLSL 500"};
  std::string spirv = JoinAllInsts(
      Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  EXPECT_FALSE(ir.source.es);
  EXPECT_EQ(ir.source.version, 450);
  EXPECT_TRUE(ir.source.known);
  EXPECT_TRUE(ir.source.hlsl);
}

}  // namespace
}  // namespace opt
}  // namespace spvtools
