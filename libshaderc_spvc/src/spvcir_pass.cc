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

#include "spvcir_pass.h"

#include "source/opt/ir_context.h"

namespace spvtools {
namespace opt {

SpvcIrPass::SpvcIrPass(spirv_cross::ParsedIR *ir) {
  ir_ = ir;
  current_function_ = nullptr;
  current_block_ = nullptr;

  auto s = ir_->spirv.data();
  assert(ir_->spirv.size() > 3 && "spirv data is too small");
  uint32_t bound = s[3];
  ir_->set_id_bounds(bound);
}

Pass::Status SpvcIrPass::Process() {
  get_module()->ForEachInst(
      [this](Instruction *inst) { GenerateSpirvCrossIR(inst); });

  assert(!current_function_ && "Function was not terminated.");
  assert(!current_block_ && "Block Was not terminated");

  return Status::SuccessWithoutChange;
}

void SpvcIrPass::GenerateSpirvCrossIR(Instruction *inst) {

  switch (inst->opcode()) {
    case SpvOpSourceContinued:
    case SpvOpSourceExtension:
    case SpvOpNop:
    case SpvOpModuleProcessed:
      break;

    case SpvOpSource: {
      auto lang =
          static_cast<spv::SourceLanguage>(inst->GetSingleWordOperand(0u));
      switch (lang) {
        case spv::SourceLanguageESSL:
          ir_->source.es = true;
          ir_->source.version = inst->GetSingleWordOperand(1u);
          ir_->source.known = true;
          ir_->source.hlsl = false;
          break;

        case spv::SourceLanguageGLSL:
          ir_->source.es = false;
          ir_->source.version = inst->GetSingleWordOperand(1u);
          ir_->source.known = true;
          ir_->source.hlsl = false;
          break;

        case spv::SourceLanguageHLSL:
          // For purposes of cross-compiling, this is GLSL 450.
          ir_->source.es = false;
          ir_->source.version = 450;
          ir_->source.known = true;
          ir_->source.hlsl = true;
          break;

        default:
          ir_->source.known = false;
          break;
      }
      break;
    }

    case SpvOpCapability: {
      auto cap = inst->GetSingleWordOperand(0u);
      assert(cap != spv::CapabilityKernel &&
             "Kernel capability not supported.");
      ir_->declared_capabilities.push_back(static_cast<spv::Capability>(cap));
      break;
    }

    case SpvOpExtension: {
      const std::string extension_name =
          reinterpret_cast<const char *>(inst->GetInOperand(0u).words.data());
      ir_->declared_extensions.push_back(std::move(extension_name));
      break;
    }

    case SpvOpMemoryModel: {
      ir_->addressing_model =
          static_cast<spv::AddressingModel>(inst->GetSingleWordInOperand(0u));
      ir_->memory_model =
          static_cast<spv::MemoryModel>(inst->GetSingleWordInOperand(1u));
      break;
    }

    case SpvOpEntryPoint: {
      auto function_id = inst->GetSingleWordInOperand(1u);
      auto execution_mode =
          static_cast<spv::ExecutionModel>(inst->GetSingleWordInOperand(0u));

      const char *entry_name =
          reinterpret_cast<const char *>(inst->GetInOperand(2u).words.data());

      auto itr = ir_->entry_points.insert(std::make_pair(
          function_id, spirv_cross::SPIREntryPoint(function_id, execution_mode,
                                                   entry_name)));
      auto &e = itr.first->second;
      ir_->set_name(function_id, e.name);

      for (uint32_t i = 3; i < inst->NumInOperands(); ++i) {
        e.interface_variables.push_back(inst->GetSingleWordInOperand(i));
      }

      if (!ir_->default_entry_point) ir_->default_entry_point = function_id;
      break;
    }

    case SpvOpTypeVoid: {
      auto id = inst->result_id();
      auto &type = set<spirv_cross::SPIRType>(id);
      type.basetype = spirv_cross::SPIRType::Void;
      break;
    }

    case SpvOpTypeFunction: {
      auto id = inst->result_id();
      auto return_type = inst->GetSingleWordInOperand(0u);

      // Reading Parameter 0 Type, Parameter 1 Type, ...
      // Starting i at 1 to skip over the function return type parameter.
      auto &func = set<spirv_cross::SPIRFunctionPrototype>(id, return_type);
      for (uint32_t i = 1; i < inst->NumInOperands(); ++i) {
        func.parameter_types.push_back(inst->GetSingleWordInOperand(i));
      }

      break;
    }

    case SpvOpFunction: {
      auto result = inst->type_id();
      auto id = inst->result_id();
      auto type = inst->GetSingleWordInOperand(1u);

      assert(!current_function_ &&
             "Must end a function before starting a new one!");

      current_function_ = &set<spirv_cross::SPIRFunction>(id, result, type);
      break;
    }

      // Blocks
    case SpvOpLabel: {
      // OpLabel always starts a block.
      assert(current_function_ && "Blocks cannot exist outside functions!");

      uint32_t id = inst->result_id();

      current_function_->blocks.push_back(id);

      if (!current_function_->entry_block) {
        current_function_->entry_block = id;
      }
      assert(!current_block_ &&
             "Cannot start a block before ending the current block.");

      current_block_ = &set<spirv_cross::SPIRBlock>(id);
      break;
    }

    case SpvOpReturn: {
      assert(current_block_ && "Trying to end a non-existing block.");
      // TODO (sarahM0): refactor this into TerminateBlock( ... terminator type
      // ...), which also resets current_block_ to nullptr. Once having one more
      // terminator case.

      current_block_->terminator = spirv_cross::SPIRBlock::Return;
      current_block_ = nullptr;
      break;
    }

    case SpvOpFunctionEnd: {
      // Very specific error message, but seems to come up quite often.
      assert(!current_block_ &&
             "Cannot end a function before ending the current block.\n"
             "Likely cause: If this SPIR-V was created from glslang HLSL, "
             "make sure the entry point is valid.");
      current_function_ = nullptr;
      break;
    }

    default:
      break;
  }
}

}  // namespace opt
}  // namespace spvtools
