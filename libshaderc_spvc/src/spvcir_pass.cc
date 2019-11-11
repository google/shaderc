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
  status_ = Status::SuccessWithoutChange;
}

bool SpvcIrPass::CheckConditionAndSetErrorMessage(bool condition,
                                                  std::string message) {
  if (!condition) {
    status_ = Status::Failure;
    if (consumer()) {
      consumer()(SPV_MSG_ERROR, 0, {0, 0, 0}, message.c_str());
    } else {
      assert(condition && message.c_str());
    }
    return false;
  } else {
    return true;
  }
}

Pass::Status SpvcIrPass::Process() {
  auto s = ir_->spirv.data();
  if (!CheckConditionAndSetErrorMessage(ir_->spirv.size() > 3,
                                        "SpvcIrPass: spirv data is too small"))
    return Status::Failure;

  uint32_t bound = s[3];
  ir_->set_id_bounds(bound);
  get_module()->ForEachInst(
      [this](Instruction *inst) {
        if (status_ != Status::SuccessWithoutChange) return;
        GenerateSpirvCrossIR(inst);
      },
      true);

  if (!CheckConditionAndSetErrorMessage(
          !current_block_,
          "SpvcIrPass: Error at the end of parsing, block was not terminated."))
    return status_;

  if (!CheckConditionAndSetErrorMessage(
          !current_function_,
          "SpvcIrPass: Error at the end of parsing, function was not "
          "terminated."))
    return status_;

  return status_;
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
          static_cast<spv::SourceLanguage>(inst->GetSingleWordInOperand(0u));
      switch (lang) {
        case spv::SourceLanguageESSL:
          ir_->source.es = true;
          ir_->source.version = inst->GetSingleWordInOperand(1u);
          ir_->source.known = true;
          ir_->source.hlsl = false;
          break;

        case spv::SourceLanguageGLSL:
          ir_->source.es = false;
          ir_->source.version = inst->GetSingleWordInOperand(1u);
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

      if (!CheckConditionAndSetErrorMessage(
              cap != spv::CapabilityKernel,
              "SpvcIrPass: Error while parsing OpCapability, "
              "kernel capability not supported."))
        return;
      ir_->declared_capabilities.push_back(static_cast<spv::Capability>(cap));
      break;
    }

    case SpvOpExtension: {
      const std::string extension_name =
          reinterpret_cast<const char *>(inst->GetInOperand(0u).words.data());
      ir_->declared_extensions.push_back(std::move(extension_name));
      break;
    }

    // opcode:11
    case SpvOpExtInstImport: {
      uint32_t id = inst->result_id();
      std::string ext(
          reinterpret_cast<const char *>(inst->GetInOperand(0u).words.data()));
      if (ext == "GLSL.std.450")
        set<spirv_cross::SPIRExtension>(id, spirv_cross::SPIRExtension::GLSL);
      else if (ext == "DebugInfo")
        set<spirv_cross::SPIRExtension>(
            id, spirv_cross::SPIRExtension::SPV_debug_info);
      else if (ext == "SPV_AMD_shader_ballot")
        set<spirv_cross::SPIRExtension>(
            id, spirv_cross::SPIRExtension::SPV_AMD_shader_ballot);
      else if (ext == "SPV_AMD_shader_explicit_vertex_parameter")
        set<spirv_cross::SPIRExtension>(
            id, spirv_cross::SPIRExtension::
                    SPV_AMD_shader_explicit_vertex_parameter);
      else if (ext == "SPV_AMD_shader_trinary_minmax")
        set<spirv_cross::SPIRExtension>(
            id, spirv_cross::SPIRExtension::SPV_AMD_shader_trinary_minmax);
      else if (ext == "SPV_AMD_gcn_shader")
        set<spirv_cross::SPIRExtension>(
            id, spirv_cross::SPIRExtension::SPV_AMD_gcn_shader);
      else {
        // spirv-cross comment:
        // Other SPIR-V extensions which have ExtInstrs are currently not
        // supported.
        // TODO(sarahM0): figure out which ones are not supported and try to add
        // them.
        if (!CheckConditionAndSetErrorMessage(
                false,
                "SpvcIrPass: Error while parsing "
                "OpExtInstImport, SPIRV extension "
                "not supported: " +
                    ext))
          return;
      }
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

    // opcode: 71
    case SpvOpDecorate:
    // opcode: 332
    case SpvOpDecorateId: {
      // spirv-cross comment:
      // OpDecorateId technically supports an array of arguments, but our only
      // supported decorations are single uint, so merge decorate and
      // decorate-id here.
      // TODO(sarahM0): investigate if this limitation is acceptable.
      uint32_t id = inst->GetSingleWordInOperand(0u);

      auto decoration =
          static_cast<spv::Decoration>(inst->GetSingleWordInOperand(1u));
      if (inst->NumInOperands() > 2) {
        // instruction offset + length = offset_ + 1 + inst->NumOpreandWords()
        if (!CheckConditionAndSetErrorMessage(
                offset_ + 1 + inst->NumOperandWords() < ir_->spirv.size(),
                "SpvcIrPass: Error while parsing OpDecorate/OpDecorateId, "
                "reading out of spirv.data() bound"))
          return;
        // The extra operand of the decoration (Literal, Literal, …) are at
        // instruction offset + 2 (skipping <id>Target, Decoration)
        ir_->meta[id].decoration_word_offset[decoration] =
            uint32_t((&ir_->spirv[offset_ + 1] + 2) - ir_->spirv.data());
        ir_->set_decoration(id, decoration, inst->GetSingleWordInOperand(2u));
      } else {
        ir_->set_decoration(id, decoration);
      }

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

      if (!CheckConditionAndSetErrorMessage(
              !current_function_,
              "SpvcIrPass: Error while parsing OpFunction, must end a function "
              "before starting a new one!"))
        return;

      current_function_ = &set<spirv_cross::SPIRFunction>(id, result, type);
      break;
    }

    // Variable declaration
    // opcode: 59
    // spirv-cross comment:
    // All variables are essentially pointers with a storage qualifier.
    case SpvOpVariable: {
      uint32_t type = inst->type_id();
      uint32_t id = inst->result_id();
      auto storage =
          static_cast<spv::StorageClass>(inst->GetSingleWordInOperand(0u));
      uint32_t initializer =
          inst->NumInOperands() == 2 ? inst->GetSingleWordInOperand(1u) : 0;

      if (storage == spv::StorageClassFunction) {
        if (!CheckConditionAndSetErrorMessage(
                current_function_,
                "SpvcIrPass: Error while parsing OpVariable, "
                "no function currently in scope"))
          return;
        current_function_->add_local_variable(id);
      }

      set<spirv_cross::SPIRVariable>(id, type, storage, initializer);

      // spirv-cross comment:
      // hlsl based shaders don't have those decorations. force them and then
      // reset when reading/writing images
      auto &ttype = get<spirv_cross::SPIRType>(type);
      if (ttype.basetype == spirv_cross::SPIRType::BaseType::Image) {
        ir_->set_decoration(id, spv::DecorationNonWritable);
        ir_->set_decoration(id, spv::DecorationNonReadable);
      }

      break;
    }

    // opcode: 21
    case SpvOpTypeInt: {
      uint32_t id = inst->result_id();
      uint32_t width = inst->GetSingleWordInOperand(0u);
      bool signedness = inst->GetSingleWordInOperand(1u) != 0;
      auto &type = set<spirv_cross::SPIRType>(id);
      type.basetype = signedness ? spirv_cross::to_signed_basetype(width)
                                 : spirv_cross::to_unsigned_basetype(width);
      type.width = width;
      break;
    }

    // opcode: 43
    case SpvOpConstant: {
      uint32_t id = inst->result_id();
      auto &type = get<spirv_cross::SPIRType>(inst->type_id());

      // TODO(sarahM0): floating-point numbers of IEEE 754 are of length 16
      // bits, 32 bits, 64 bits, and any multiple of 32 bits ≥128
      // Ask whether we need to support longer than 64 bits
      if (type.width > 32) {
        uint64_t combined_words = inst->GetInOperand(0u).words[1];
        combined_words = combined_words << 32;
        combined_words |= inst->GetInOperand(0u).words[0];
        set<spirv_cross::SPIRConstant>(id, inst->type_id(), combined_words,
                                       inst->opcode() == SpvOpSpecConstant);
      } else {
        set<spirv_cross::SPIRConstant>(id, inst->type_id(),
                                       inst->GetSingleWordInOperand(0u),
                                       inst->opcode() == SpvOpSpecConstant);
      }
      break;
    }

    // opcode: 32
    case SpvOpTypePointer: {
      uint32_t id = inst->result_id();

      auto &base = get<spirv_cross::SPIRType>(inst->GetSingleWordInOperand(1u));
      auto &ptrbase = set<spirv_cross::SPIRType>(id);

      ptrbase = base;
      ptrbase.pointer = true;
      ptrbase.pointer_depth++;
      ptrbase.storage =
          static_cast<spv::StorageClass>(inst->GetSingleWordInOperand(0u));

      if (ptrbase.storage == spv::StorageClassAtomicCounter)
        ptrbase.basetype = spirv_cross::SPIRType::AtomicCounter;

      ptrbase.parent_type = inst->GetSingleWordInOperand(1u);

      // spirv-cross comment:
      // Do NOT set ptrbase.self!
      break;
    }

      // Blocks
    case SpvOpLabel: {
      // OpLabel always starts a block.
      if (!CheckConditionAndSetErrorMessage(
              current_function_,
              "SpvcIrPass: Error while parsing OpLable, blocks "
              "cannot exist outside functions!"))
        return;

      uint32_t id = inst->result_id();

      current_function_->blocks.push_back(id);

      if (!current_function_->entry_block) {
        current_function_->entry_block = id;
      }
      if (!CheckConditionAndSetErrorMessage(
              !current_block_,
              "SpvcIrPass: Error while parsing OpLable, cannot "
              "start a block before ending the current block."))
        return;

      current_block_ = &set<spirv_cross::SPIRBlock>(id);
      break;
    }

    case SpvOpReturn: {
      if (!CheckConditionAndSetErrorMessage(
              current_block_,
              "SpvcIrPass: Error while parsing OpReturn, "
              "trying to end a non-existing block."))
        return;
      // TODO (sarahM0): refactor this into TerminateBlock( ... terminator
      // type
      // ...), which also resets current_block_ to nullptr. Once having one
      // more terminator case.

      current_block_->terminator = spirv_cross::SPIRBlock::Return;
      current_block_ = nullptr;
      break;
    }

    case SpvOpFunctionEnd: {
      // Very specific error message, but seems to come up quite often.
      if (!CheckConditionAndSetErrorMessage(
              !current_block_,
              "SpvcIrPass: Error while parsing OpFunctionEnd, cannot end a "
              "function before ending the current block.\n"
              "Likely cause: If this SPIR-V was created from glslang HLSL, "
              "make sure the entry point is valid."))
        return;
      current_function_ = nullptr;
      break;
    }

    case SpvOpStore: {
      if (!CheckConditionAndSetErrorMessage(
              current_block_,
              "SpvcIrPass: Currently no block to insert opcode."))
        return;
      spirv_cross::Instruction instr = {};
      instr.op = inst->opcode();
      instr.count = inst->NumOperandWords() + 1;
      instr.offset = offset_ + 1;
      instr.length = instr.count - 1;

      if (!CheckConditionAndSetErrorMessage(
              instr.count != 0,
              "SpvcIrPass: SPIR-V instructions cannot consume "
              "0 words. Invalid SPIR-V file."))
        return;
      if (!CheckConditionAndSetErrorMessage(
              offset_ <= ir_->spirv.size(),
              "SpvcIrPass: SPIR-V instruction goes out of bounds."))
        return;
      current_block_->ops.push_back(instr);
      break;
    }

    default: {
      if (!CheckConditionAndSetErrorMessage(
              false,
              "SpvcIrPass: Instruction not supported in spvcir parser: "
              "opcode " +
                  std::to_string(inst->opcode())))
        return;
      break;
    }
  }
  offset_ += inst->NumOperandWords() + 1;
  return;
}

}  // namespace opt
}  // namespace spvtools
