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

static bool decoration_is_string(spv::Decoration decoration) {
  switch (decoration) {
    case spv::DecorationHlslSemanticGOOGLE:
      return true;

    default:
      return false;
  }
}

void SpvcIrPass::make_constant_null(uint32_t id, uint32_t type) {
  // TODO(sarahM0): Add more tests
  auto &constant_type = get<spirv_cross::SPIRType>(type);

  if (constant_type.pointer) {
    auto &constant = set<spirv_cross::SPIRConstant>(id, type);
    constant.make_null(constant_type);
  } else if (!constant_type.array.empty()) {
    if (!CheckConditionAndSetErrorMessage(
            constant_type.parent_type,
            "constant type parent shouldn't be empty"))
      return;
    uint32_t parent_id = ir_->increase_bound_by(1);
    make_constant_null(parent_id, constant_type.parent_type);

    if (!CheckConditionAndSetErrorMessage(
            constant_type.array_size_literal.back(),
            "Array size of OpConstantNull must be a literal."))
      return;

    spirv_cross::SmallVector<uint32_t> elements(constant_type.array.back());
    for (uint32_t i = 0; i < constant_type.array.back(); i++)
      elements[i] = parent_id;
    set<spirv_cross::SPIRConstant>(id, type, elements.data(),
                                   uint32_t(elements.size()), false);
  } else if (!constant_type.member_types.empty()) {
    uint32_t member_ids =
        ir_->increase_bound_by(uint32_t(constant_type.member_types.size()));
    spirv_cross::SmallVector<uint32_t> elements(
        constant_type.member_types.size());
    for (uint32_t i = 0; i < constant_type.member_types.size(); i++) {
      make_constant_null(member_ids + i, constant_type.member_types[i]);
      elements[i] = member_ids + i;
    }
    set<spirv_cross::SPIRConstant>(id, type, elements.data(),
                                   uint32_t(elements.size()), false);
  } else {
    auto &constant = set<spirv_cross::SPIRConstant>(id, type);
    constant.make_null(constant_type);
  }
}

bool SpvcIrPass::types_are_logically_equivalent(
    const spirv_cross::SPIRType &a, const spirv_cross::SPIRType &b) const {
  if (a.basetype != b.basetype) return false;
  if (a.width != b.width) return false;
  if (a.vecsize != b.vecsize) return false;
  if (a.columns != b.columns) return false;
  if (a.array.size() != b.array.size()) return false;

  size_t array_count = a.array.size();
  if (array_count && memcmp(a.array.data(), b.array.data(),
                            array_count * sizeof(uint32_t)) != 0)
    return false;

  if (a.basetype == spirv_cross::SPIRType::Image ||
      a.basetype == spirv_cross::SPIRType::SampledImage) {
    if (memcmp(&a.image, &b.image, sizeof(spirv_cross::SPIRType::Image)) != 0)
      return false;
  }

  if (a.member_types.size() != b.member_types.size()) return false;

  size_t member_types = a.member_types.size();
  for (size_t i = 0; i < member_types; i++) {
    if (!types_are_logically_equivalent(
            get<spirv_cross::SPIRType>(a.member_types[i]),
            get<spirv_cross::SPIRType>(b.member_types[i])))
      return false;
  }

  return true;
}

// Returns the string representation of the data starting at the index of the
// given instruction, assuming data is null terminated.
inline std::string GetWordsAsString(uint32_t index, Instruction *inst) {
  return reinterpret_cast<const char *>(inst->GetInOperand(index).words.data());
}

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
  }
  return condition;
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

    // opcode: 1
    case SpvOpUndef: {
      uint32_t result_type = inst->type_id();
      uint32_t id = inst->result_id();
      set<spirv_cross::SPIRUndef>(id, result_type);

      if (current_block_) {
        spirv_cross::Instruction instr = {};
        instr.op = inst->opcode();
        instr.count = inst->NumOperandWords() + 1;
        instr.offset = offset_ + 1;
        instr.length = instr.count - 1;

        current_block_->ops.push_back(instr);
      }
      break;
    }

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
      auto cap = inst->GetSingleWordInOperand(0u);

      if (!CheckConditionAndSetErrorMessage(
              cap != spv::CapabilityKernel,
              "SpvcIrPass: Error while parsing OpCapability, "
              "kernel capability not supported."))
        return;
      ir_->declared_capabilities.push_back(static_cast<spv::Capability>(cap));
      break;
    }

    case SpvOpExtension: {
      const std::string extension_name = GetWordsAsString(0u, inst);
      ir_->declared_extensions.push_back(extension_name);
      break;
    }

    // opcode: 5
    case SpvOpName: {
      uint32_t id = inst->GetSingleWordInOperand(0u);
      ir_->set_name(id, GetWordsAsString(1u, inst));
      break;
    }

    // opcode:6
    case SpvOpMemberName: {
      uint32_t id = inst->GetSingleWordInOperand(0u);
      uint32_t member = inst->GetSingleWordInOperand(1u);
      ir_->set_member_name(id, member, GetWordsAsString(2u, inst));
      break;
    }

    // opcode: 7
    case SpvOpString: {
      set<spirv_cross::SPIRString>(inst->result_id(),
                                   GetWordsAsString(0u, inst));
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
        // TODO(sarahM0): figure out which ones are not supported and try to
        // add them.
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

    // opcode: 12
    // TODO(sarahM0): Figure out how to test this.
    case SpvOpExtInst: {
      // spirv_cross comment:
      // The SPIR-V debug information extended instructions might come at
      // global scope.
      spirv_cross::Instruction instruction = {};
      instruction.op = inst->opcode();
      instruction.count = inst->NumOperandWords() + 1;
      instruction.offset = offset_ + 1;
      instruction.length = instruction.count - 1;
      if (current_block_) current_block_->ops.push_back(instruction);
      break;
    }

    // opcode: 14
    case SpvOpMemoryModel: {
      ir_->addressing_model =
          static_cast<spv::AddressingModel>(inst->GetSingleWordInOperand(0u));
      ir_->memory_model =
          static_cast<spv::MemoryModel>(inst->GetSingleWordInOperand(1u));
      break;
    }

    // opcode:15
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

    // opcode: 16
    case SpvOpExecutionMode: {
      auto &execution = ir_->entry_points[inst->GetSingleWordInOperand(0u)];
      auto mode =
          static_cast<SpvExecutionMode>(inst->GetSingleWordInOperand(1u));
      execution.flags.set(mode);

      switch (mode) {
        case SpvExecutionModeInvocations: {
          execution.invocations = inst->GetSingleWordInOperand(2u);
          break;
        }

        case SpvExecutionModeLocalSize: {
          execution.workgroup_size.x = inst->GetSingleWordInOperand(2u);
          execution.workgroup_size.y = inst->GetSingleWordInOperand(3u);
          execution.workgroup_size.z = inst->GetSingleWordInOperand(4u);
          break;
        }

        case SpvExecutionModeOutputVertices: {
          execution.output_vertices = inst->GetSingleWordInOperand(2u);
          break;
        }

        default: {
          break;
          // Other executions modes are set as part of "Bitset flags" member
          // of struct SPIREntryPoint in the compiler
        }
      }
      break;
    }

    // opcode: 71
    case SpvOpDecorate:
    // opcode: 332
    case SpvOpDecorateId: {
      // spirv-cross comment:
      // OpDecorateId technically supports an array of arguments, but our
      // only supported decorations are single uint, so merge decorate and
      // decorate-id here.
      // TODO(sarahM0): investigate if this limitation is acceptable.
      uint32_t id = inst->GetSingleWordInOperand(0u);

      auto decoration =
          static_cast<spv::Decoration>(inst->GetSingleWordInOperand(1u));
      if (inst->NumInOperands() > 2) {
        // instruction offset + length = offset_ + 1 +
        // inst->NumOpreandWords()
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

    // opcode: 72
    case SpvOpMemberDecorate: {
      uint32_t id = inst->GetSingleWordInOperand(0u);
      uint32_t member = inst->GetSingleWordInOperand(1u);
      auto decoration =
          static_cast<spv::Decoration>(inst->GetSingleWordInOperand(2u));
      if (inst->NumInOperands() >= 4)
        ir_->set_member_decoration(id, member, decoration,
                                   inst->GetSingleWordInOperand(3u));
      else
        ir_->set_member_decoration(id, member, decoration);
      break;
    }

    // opcode: 73
    case SpvOpDecorationGroup: {
      // spirv-cross comment:
      // Noop, this simply means an ID should be a collector of decorations.
      // The meta array is already a flat array of decorations which will
      // contain the relevant decorations.
      break;
    }

    // opcode: 74
    case SpvOpGroupDecorate: {
      uint32_t group_id = inst->GetSingleWordInOperand(0u);
      auto &decorations = ir_->meta[group_id].decoration;
      auto &flags = decorations.decoration_flags;

      // Copies decorations from one ID to another. Only copy decorations which
      // are set in the group, i.e., we cannot just copy the meta structure
      // directly.
      for (uint32_t i = 1; i < inst->NumInOperands(); i++) {
        uint32_t target = inst->GetSingleWordInOperand(i);
        flags.for_each_bit([&](uint32_t bit) {
          auto decoration = static_cast<spv::Decoration>(bit);

          if (decoration_is_string(decoration)) {
            ir_->set_decoration_string(
                target, decoration,
                ir_->get_decoration_string(group_id, decoration));
          } else {
            ir_->meta[target].decoration_word_offset[decoration] =
                ir_->meta[group_id].decoration_word_offset[decoration];
            ir_->set_decoration(target, decoration,
                                ir_->get_decoration(group_id, decoration));
          }
        });
      }
      break;
    }

    // opcode: 75
    case SpvOpGroupMemberDecorate: {
      uint32_t group_id = inst->GetSingleWordInOperand(0u);
      auto &flags = ir_->meta[group_id].decoration.decoration_flags;

      // Copies decorations from one ID to another. Only copy decorations which
      // are set in the group, i.e., we cannot just copy the meta structure
      // directly.
      for (uint32_t i = 1; i + 1 < inst->NumInOperands(); i += 2) {
        uint32_t target = inst->GetSingleWordInOperand(i + 0u);
        uint32_t index = inst->GetSingleWordInOperand(i + 1u);
        flags.for_each_bit([&](uint32_t bit) {
          auto decoration = static_cast<spv::Decoration>(bit);

          if (decoration_is_string(decoration))
            ir_->set_member_decoration_string(
                target, index, decoration,
                ir_->get_decoration_string(group_id, decoration));
          else
            ir_->set_member_decoration(
                target, index, decoration,
                ir_->get_decoration(group_id, decoration));
        });
      }
      break;
    }

    // opcode: 5632
    case SpvOpDecorateStringGOOGLE: {
      uint32_t id = inst->GetSingleWordInOperand(0u);
      auto decoration =
          static_cast<spv::Decoration>(inst->GetSingleWordInOperand(1u));
      ir_->set_decoration_string(id, decoration, GetWordsAsString(2u, inst));
      break;
    }

    // opcode: 5633
    case SpvOpMemberDecorateStringGOOGLE: {
      uint32_t id = inst->GetSingleWordInOperand(0u);
      uint32_t member = inst->GetSingleWordInOperand(1u);
      auto decoration =
          static_cast<spv::Decoration>(inst->GetSingleWordInOperand(2u));
      ir_->set_member_decoration_string(id, member, decoration,
                                        GetWordsAsString(3u, inst));
      break;
    }

      // Basic type cases
      // opcode: 19
    case SpvOpTypeVoid: {
      auto id = inst->result_id();
      auto &type = set<spirv_cross::SPIRType>(id);
      type.basetype = spirv_cross::SPIRType::Void;
      break;
    }

    // opcode: 20
    case SpvOpTypeBool: {
      uint32_t id = inst->result_id();
      auto &type = set<spirv_cross::SPIRType>(id);
      type.basetype = spirv_cross::SPIRType::Boolean;
      type.width = 1;
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

    // opcode: 22
    case SpvOpTypeFloat: {
      uint32_t id = inst->result_id();
      auto &type = set<spirv_cross::SPIRType>(id);
      uint32_t width = inst->GetSingleWordInOperand(0u);
      if (width == 64)
        type.basetype = spirv_cross::SPIRType::Double;
      else if (width == 32)
        type.basetype = spirv_cross::SPIRType::Float;
      else if (width == 16)
        type.basetype = spirv_cross::SPIRType::Half;
      else
        CheckConditionAndSetErrorMessage(
            false, "Unrecognized bit-width of floating point type.");

      type.width = width;
      break;
    }

    // opcode: 23
    // spirv-cross comment:
    // Build composite types by "inheriting".
    // NOTE: The self member is also copied! For pointers and array modifiers
    // this is a good thing since we can refer to decorations on pointee
    // classes which is needed for UBO/SSBO, I/O blocks in geometry/tess etc.
    case SpvOpTypeVector: {
      uint32_t id = inst->result_id();
      // TODO(sarahM0): ask if Column Count, in operand one, can be
      // double-word.
      uint32_t vecsize = inst->GetSingleWordInOperand(1u);

      auto &base = get<spirv_cross::SPIRType>(inst->GetSingleWordInOperand(0u));
      auto &vecbase = set<spirv_cross::SPIRType>(id);

      vecbase = base;
      vecbase.vecsize = vecsize;
      vecbase.self = id;
      vecbase.parent_type = inst->GetSingleWordInOperand(0u);
      break;
    }

    // opcode: 24
    case SpvOpTypeMatrix: {
      uint32_t id = inst->result_id();
      uint32_t colcount = inst->GetSingleWordInOperand(1u);

      auto &base = get<spirv_cross::SPIRType>(inst->GetSingleWordInOperand(0u));
      auto &matrixbase = set<spirv_cross::SPIRType>(id);

      matrixbase = base;
      matrixbase.columns = colcount;
      matrixbase.self = id;
      matrixbase.parent_type = inst->GetSingleWordInOperand(0u);
      break;
    }

    // opcode: 25
    case SpvOpTypeImage: {
      uint32_t id = inst->result_id();
      auto &type = set<spirv_cross::SPIRType>(id);
      type.basetype = spirv_cross::SPIRType::Image;
      type.image.type = inst->GetSingleWordInOperand(0u);
      type.image.dim = static_cast<spv::Dim>(inst->GetSingleWordInOperand(1u));
      type.image.depth = inst->GetSingleWordInOperand(2u) == 1;
      type.image.arrayed = inst->GetSingleWordInOperand(3u) != 0;
      type.image.ms = inst->GetSingleWordInOperand(4u) != 0;
      type.image.sampled = inst->GetSingleWordInOperand(5u);
      type.image.format =
          static_cast<spv::ImageFormat>(inst->GetSingleWordInOperand(6u));
      type.image.access = (inst->NumInOperands() > 7)
                              ? static_cast<spv::AccessQualifier>(
                                    inst->GetSingleWordInOperand(7u))
                              : spv::AccessQualifierMax;
      break;
    }

    // opcode: 26
    case SpvOpTypeSampler: {
      uint32_t id = inst->result_id();
      auto &type = set<spirv_cross::SPIRType>(id);
      type.basetype = spirv_cross::SPIRType::Sampler;
      break;
    }

    // opcode: 27
    case SpvOpTypeSampledImage: {
      uint32_t id = inst->result_id();
      uint32_t imagetype = inst->GetSingleWordInOperand(0u);
      auto &type = set<spirv_cross::SPIRType>(id);
      type = get<spirv_cross::SPIRType>(imagetype);
      type.basetype = spirv_cross::SPIRType::SampledImage;
      type.self = id;
      break;
    }

    // opcode: 28
    case SpvOpTypeArray: {
      uint32_t id = inst->result_id();
      auto &arraybase = set<spirv_cross::SPIRType>(id);

      uint32_t tid = inst->GetSingleWordInOperand(0u);
      auto &base = get<spirv_cross::SPIRType>(tid);

      arraybase = base;
      arraybase.parent_type = tid;

      uint32_t cid = inst->GetSingleWordInOperand(1u);
      ir_->mark_used_as_array_length(cid);
      auto *c = maybe_get<spirv_cross::SPIRConstant>(cid);
      bool literal = c && !c->specialization;

      arraybase.array_size_literal.push_back(literal);
      arraybase.array.push_back(literal ? c->scalar() : cid);
      // spirv-cross comment:
      // Do NOT set arraybase.self!
      break;
    }

    // opcode: 29
    case SpvOpTypeRuntimeArray: {
      auto id = inst->result_id();
      auto tid = inst->GetSingleWordInOperand(0u);
      auto &base = get<spirv_cross::SPIRType>(tid);
      auto &arraybase = set<spirv_cross::SPIRType>(id);
      arraybase = base;
      arraybase.array.push_back(0);
      arraybase.array_size_literal.push_back(true);
      arraybase.parent_type = tid;
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

    // opcode: 33
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

    // opcode: 39
    case SpvOpTypeForwardPointer: {
      uint32_t id = inst->GetSingleWordInOperand(0u);
      auto &ptrbase = set<spirv_cross::SPIRType>(id);
      ptrbase.pointer = true;
      ptrbase.pointer_depth++;
      ptrbase.storage =
          static_cast<spv::StorageClass>(inst->GetSingleWordInOperand(1u));
      // TODO(sarahM0): add test for with AtomicCounter storage class
      if (ptrbase.storage == spv::StorageClassAtomicCounter)
        ptrbase.basetype = spirv_cross::SPIRType::AtomicCounter;

      break;
    }

      // opcode: 5341
    case SpvOpTypeAccelerationStructureNV: {
      uint32_t id = inst->result_id();
      auto &type = set<spirv_cross::SPIRType>(id);
      type.basetype = spirv_cross::SPIRType::AccelerationStructureNV;
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
      // hlsl based shaders don't have those decorations. force them and
      // then reset when reading/writing images
      auto &ttype = get<spirv_cross::SPIRType>(type);
      if (ttype.basetype == spirv_cross::SPIRType::BaseType::Image) {
        ir_->set_decoration(id, spv::DecorationNonWritable);
        ir_->set_decoration(id, spv::DecorationNonReadable);
      }

      break;
    }

      // opcode: 30
    case SpvOpTypeStruct: {
      uint32_t id = inst->result_id();
      auto &type = set<spirv_cross::SPIRType>(id);
      type.basetype = spirv_cross::SPIRType::Struct;
      for (uint32_t i = 0; i < inst->NumInOperands(); i++) {
        type.member_types.push_back(inst->GetSingleWordInOperand(i));
      }

      // TODO(sarahM0): ask about aliasing? figure out what is happening in
      // this loop.
      bool consider_aliasing = !ir_->get_name(type.self).empty();
      if (consider_aliasing) {
        for (auto &other : global_struct_cache_) {
          if (ir_->get_name(type.self) == ir_->get_name(other) &&
              types_are_logically_equivalent(
                  type, get<spirv_cross::SPIRType>(other))) {
            type.type_alias = other;
            break;
          }
        }

        if (type.type_alias == spirv_cross::TypeID(0))
          global_struct_cache_.push_back(id);
      }
      break;
    }

    // Constant Cases
    // opcode: 50
    case SpvOpSpecConstant:
    // opcode: 43
    case SpvOpConstant: {
      uint32_t id = inst->result_id();
      auto &type = get<spirv_cross::SPIRType>(inst->type_id());

      // TODO(sarahM0): floating-point numbers of IEEE 754 are of length 16
      // bits, 32 bits, 64 bits, and any multiple of 32 bits up to 128 bits
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

      // opcode: 49
    case SpvOpSpecConstantFalse:
    // opcode: 42
    case SpvOpConstantFalse: {
      uint32_t id = inst->result_id();
      set<spirv_cross::SPIRConstant>(id, inst->type_id(), 0u,
                                     inst->opcode() == SpvOpSpecConstantFalse);
      break;
    }

    // opcode: 48
    case SpvOpSpecConstantTrue:
    // opcode: 41
    case SpvOpConstantTrue: {
      uint32_t id = inst->result_id();
      set<spirv_cross::SPIRConstant>(id, inst->type_id(), 1u,
                                     inst->opcode() == SpvOpSpecConstantTrue);
      break;
    }

    // opcode: 46
    case SpvOpConstantNull: {
      uint32_t id = inst->result_id();
      uint32_t type = inst->type_id();
      make_constant_null(id, type);
      break;
    }

    // opcode: 51
    case SpvOpSpecConstantComposite:
    // opcode: 44
    case SpvOpConstantComposite: {
      uint32_t id = inst->result_id();
      uint32_t type = inst->type_id();

      auto &ctype = get<spirv_cross::SPIRType>(type);

      // spirv-cross comment:
      // We can have constants which are structs and arrays.
      // In this case, our SPIRConstant will be a list of other
      // SPIRConstant ids which we can refer to.
      if (ctype.basetype == spirv_cross::SPIRType::Struct ||
          !ctype.array.empty()) {
        set<spirv_cross::SPIRConstant>(
            id, type, (&ir_->spirv[offset_ + 1u] + 2u), inst->NumInOperands(),
            inst->opcode() == SpvOpSpecConstantComposite);
      } else {
        if (!CheckConditionAndSetErrorMessage(
                inst->NumInOperands() < 5,
                "OpConstantComposite only supports 1, 2, 3 and 4 elements."))
          return;

        spirv_cross::SPIRConstant remapped_constant_ops[4];
        const spirv_cross::SPIRConstant *c[4];
        for (uint32_t i = 0; i < inst->NumInOperands(); i++) {
          // spirv-cross comment:
          // Specialization constants operations can also be part of this.
          // We do not know their value, so any attempt to query
          // SPIRConstant later will fail. We can only propagate the ID of the
          // expression and use to_expression on it.
          auto *constant_op = maybe_get<spirv_cross::SPIRConstantOp>(
              inst->GetSingleWordInOperand(i));
          auto *undef_op = maybe_get<spirv_cross::SPIRUndef>(
              inst->GetSingleWordInOperand(i));
          if (constant_op) {
            if (!CheckConditionAndSetErrorMessage(
                    inst->opcode() != SpvOpConstantComposite,
                    "Specialization constant operation used in "
                    "OpConstantComposite."))
              return;

            remapped_constant_ops[i].make_null(
                get<spirv_cross::SPIRType>(constant_op->basetype));
            remapped_constant_ops[i].self = constant_op->self;
            remapped_constant_ops[i].constant_type = constant_op->basetype;
            remapped_constant_ops[i].specialization = true;
            c[i] = &remapped_constant_ops[i];
          } else if (undef_op) {
            // spirv-cross comment:
            // Undefined, just pick 0.
            remapped_constant_ops[i].make_null(
                get<spirv_cross::SPIRType>(undef_op->basetype));
            remapped_constant_ops[i].constant_type = undef_op->basetype;
            c[i] = &remapped_constant_ops[i];
          } else {
            c[i] = &get<spirv_cross::SPIRConstant>(
                inst->GetSingleWordInOperand(i));
          }
        }
        set<spirv_cross::SPIRConstant>(
            id, type, c, inst->NumInOperands(),
            inst->opcode() == SpvOpSpecConstantComposite);
      }
      break;
    }

    case SpvOpSpecConstantOp: {
      uint32_t result_type = inst->type_id();
      uint32_t id = inst->result_id();
      auto spec_op = static_cast<spv::Op>(inst->GetSingleWordInOperand(0u));

      set<spirv_cross::SPIRConstantOp>(id, result_type, spec_op,
                                       (&ir_->spirv[offset_ + 1u] + 3u),
                                       inst->NumInOperands() - 1);
      break;
    }

      // Control Flow cases

    // opcode: 245
    // spirv-cross comment:
    // OpPhi is a fairly magical opcode.
    // It selects temporary variables based on which parent block we *came
    // from*. In high-level languages we can "de-SSA" by creating a function
    // local, and flush out temporaries to this function-local variable to
    // emulate SSA Phi.
    case SpvOpPhi: {
      if (!CheckConditionAndSetErrorMessage(
              current_function_,
              "SpvcIrPass: Trying to end a non-existing block."))
        return;

      if (!CheckConditionAndSetErrorMessage(
              current_block_, "SpvcIrPass: No block currently in scope"))
        return;

      uint32_t result_type = inst->type_id();
      uint32_t id = inst->result_id();

      // spirv-cross comment:
      // Instead of a temporary, create a new function-wide temporary with
      // this ID instead.
      auto &var = set<spirv_cross::SPIRVariable>(id, result_type,
                                                 spv::StorageClassFunction);
      var.phi_variable = true;

      current_function_->add_local_variable(id);

      for (uint32_t i = 0; i + 1 < inst->NumInOperands(); i += 2)
        current_block_->phi_variables.push_back(
            {inst->GetSingleWordInOperand(i),
             inst->GetSingleWordInOperand(i + 1), id});
      break;
    }

      // opcode: 246
    case SpvOpLoopMerge: {
      if (!CheckConditionAndSetErrorMessage(
              current_block_,
              "SpvcIrPass: Trying to end a non-existing block."))
        return;

      current_block_->merge_block = inst->GetSingleWordInOperand(0u);
      current_block_->continue_block = inst->GetSingleWordInOperand(1u);
      current_block_->merge = spirv_cross::SPIRBlock::MergeLoop;

      ir_->block_meta[current_block_->self] |=
          spirv_cross::ParsedIR::BLOCK_META_LOOP_HEADER_BIT;
      ir_->block_meta[current_block_->merge_block] |=
          spirv_cross::ParsedIR::BLOCK_META_LOOP_MERGE_BIT;

      ir_->continue_block_to_loop_header[current_block_->continue_block] =
          spirv_cross::BlockID(current_block_->self);
      // spirv-cross comment:
      // Don't add loop headers to continue blocks,
      // which would make it impossible branch into the loop header since
      // they are treated as continues.
      if (current_block_->continue_block !=
          spirv_cross::BlockID(current_block_->self))
        ir_->block_meta[current_block_->continue_block] |=
            spirv_cross::ParsedIR::BLOCK_META_CONTINUE_BIT;

      if (inst->NumInOperands() > 3) {
        if (inst->GetSingleWordInOperand(2u) & spv::LoopControlUnrollMask)
          current_block_->hint = spirv_cross::SPIRBlock::HintUnroll;
        else if (inst->GetSingleWordInOperand(2u) &
                 spv::LoopControlDontUnrollMask)
          current_block_->hint = spirv_cross::SPIRBlock::HintDontUnroll;
      }
      break;
    }

    // opcode 247
    case SpvOpSelectionMerge: {
      if (!CheckConditionAndSetErrorMessage(
              current_block_,
              "SpvcIrPass: Trying to end a non-existing block."))
        return;
      current_block_->next_block = inst->GetSingleWordInOperand(0u);
      current_block_->merge = spirv_cross::SPIRBlock::MergeSelection;
      ir_->block_meta[current_block_->next_block] |=
          spirv_cross::ParsedIR::BLOCK_META_SELECTION_MERGE_BIT;

      if (inst->NumInOperands() - 1 >= 2) {
        if (inst->GetSingleWordInOperand(1u) & spv::SelectionControlFlattenMask)
          current_block_->hint = spirv_cross::SPIRBlock::HintFlatten;
        else if (inst->GetSingleWordInOperand(1u) &
                 spv::SelectionControlDontFlattenMask)
          current_block_->hint = spirv_cross::SPIRBlock::HintDontFlatten;
      }
      break;
    }

    // opcode: 248
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

      // opcode: 249
    case SpvOpBranch: {
      if (!CheckConditionAndSetErrorMessage(
              current_block_,
              "SpvcIrPass: Trying to end a non-existing block."))
        return;

      uint32_t target = inst->GetSingleWordInOperand(0u);
      current_block_->terminator = spirv_cross::SPIRBlock::Direct;
      current_block_->next_block = target;
      current_block_ = nullptr;
      break;
    }

    // opcode: 250
    case SpvOpBranchConditional: {
      if (!CheckConditionAndSetErrorMessage(
              current_block_,
              "SpvcIrPass: Trying to end a non-existing block."))
        return;

      current_block_->condition = inst->GetSingleWordInOperand(0u);
      current_block_->true_block = inst->GetSingleWordInOperand(1u);
      current_block_->false_block = inst->GetSingleWordInOperand(2u);

      current_block_->terminator = spirv_cross::SPIRBlock::Select;
      current_block_ = nullptr;
      break;
    }

    // opcode: 251
    case SpvOpSwitch: {
      if (!CheckConditionAndSetErrorMessage(
              current_block_,
              "SpvcIrPass: Trying to end a non-existing block."))
        return;
      current_block_->terminator = spirv_cross::SPIRBlock::MultiSelect;

      current_block_->condition = inst->GetSingleWordInOperand(0u);
      current_block_->default_block = inst->GetSingleWordInOperand(1u);

      for (uint32_t i = 2; i + 1 < inst->NumInOperands(); i += 2)
        current_block_->cases.push_back({inst->GetSingleWordInOperand(i),
                                         inst->GetSingleWordInOperand(i + 1)});
      // spirv-cross comment:
      // If we jump to next block, make it break instead since we're inside a
      // switch case block at that point.
      ir_->block_meta[current_block_->next_block] |=
          spirv_cross::ParsedIR::BLOCK_META_MULTISELECT_MERGE_BIT;

      current_block_ = nullptr;
      break;
    }

    // opcode: 252
    case SpvOpKill: {
      if (!CheckConditionAndSetErrorMessage(
              current_block_,
              "SpvcIrPass: Trying to end a non-existing block."))
        return;
      current_block_->terminator = spirv_cross::SPIRBlock::Kill;
      current_block_ = nullptr;
      break;
    }

    // opcode: 253
    case SpvOpReturn: {
      if (!CheckConditionAndSetErrorMessage(
              current_block_,
              "SpvcIrPass: Error while parsing OpReturn, "
              "trying to end a non-existing block."))
        return;
      current_block_->terminator = spirv_cross::SPIRBlock::Return;
      current_block_ = nullptr;
      break;
    }

    // opcode: 254
    case SpvOpReturnValue: {
      if (!CheckConditionAndSetErrorMessage(
              current_block_,
              "SpvcIrPass: Trying to end a non-existing block."))
        return;
      current_block_->terminator = spirv_cross::SPIRBlock::Return;
      current_block_->return_value = inst->GetSingleWordInOperand(0u);
      current_block_ = nullptr;
      break;
    }

      // opcode: 255
    case SpvOpUnreachable: {
      if (!CheckConditionAndSetErrorMessage(
              current_block_,
              "SpvcIrPass: Trying to end a non-existing block. Opcode: " +
                  std::to_string(inst->opcode())))
        return;
      current_block_->terminator = spirv_cross::SPIRBlock::Unreachable;
      current_block_ = nullptr;
      break;
    }

    // Function cases:
    // opcode: 54
    case SpvOpFunction: {
      auto result = inst->type_id();
      auto id = inst->result_id();
      auto type = inst->GetSingleWordInOperand(1u);

      if (!CheckConditionAndSetErrorMessage(
              !current_function_,
              "SpvcIrPass: Must end a function "
              "before starting a new one. Opcode: " +
                  std::to_string(inst->opcode())))
        return;

      current_function_ = &set<spirv_cross::SPIRFunction>(id, result, type);
      break;
    }

    // opcode: 55
    case SpvOpFunctionParameter: {
      auto type = inst->type_id();
      auto id = inst->result_id();

      if (!CheckConditionAndSetErrorMessage(
              current_function_,
              "SpvcIrPass: OpFunctionParameter must be in a function!"))
        return;
      current_function_->add_parameter(type, id);
      set<spirv_cross::SPIRVariable>(id, type, spv::StorageClassFunction);
      break;
    }

    // opcode: 56
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

    // opcode: 8
    case SpvOpLine: {
      // OpLine might come at global scope, but we don't care about those since
      // they will not be declared in any meaningful correct order. Ignore all
      // OpLine directives which live outside a function.
      if (current_block_) {
        spirv_cross::Instruction instr = {};
        instr.op = inst->opcode();
        instr.count = inst->NumOperandWords() + 1;
        instr.offset = offset_ + 1;
        instr.length = instr.count - 1;
        current_block_->ops.push_back(instr);
      }
      // Line directives may arrive before first OpLabel.
      // Treat this as the line of the function declaration,
      // so warnings for arguments can propagate properly.
      if (current_function_) {
        // Store the first one we find and emit it before creating the function
        // prototype.
        if (current_function_->entry_line.file_id == 0) {
          current_function_->entry_line.file_id =
              inst->GetSingleWordInOperand(0u);
          current_function_->entry_line.line_literal =
              inst->GetSingleWordInOperand(1u);
        }
      }
      break;
    }

    // opcode: 317
    case SpvOpNoLine: {
      // spirv_cross comment: OpNoLine might come at global scope.
      if (current_block_) {
        spirv_cross::Instruction instr = {};
        instr.op = inst->opcode();
        instr.count = inst->NumOperandWords() + 1;
        instr.offset = offset_ + 1;
        instr.length = instr.count - 1;
        current_block_->ops.push_back(instr);
      }
      break;
    }

    default: {
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
  }
  offset_ += inst->NumOperandWords() + 1;
}

}  // namespace opt
}  // namespace spvtools
