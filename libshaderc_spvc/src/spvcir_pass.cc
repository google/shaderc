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
  // printf("opcode is: %d \n", inst->opcode());
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
      auto cap = inst->GetSingleWordInOperand(0u);
      assert(cap != spv::CapabilityKernel &&
             "Kernel capability not supported.");
      ir_->declared_capabilities.push_back(static_cast<spv::Capability>(cap));
      break;
    }

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
      else
        set<spirv_cross::SPIRExtension>(
            id, spirv_cross::SPIRExtension::Unsupported);

      // Other SPIR-V extensions which have ExtInstrs are currently not
      // supported.

      break;
    }

    case SpvOpExecutionMode: {
      auto &execution = ir_->entry_points[inst->GetSingleWordInOperand(0u)];
      auto mode =
          static_cast<SpvExecutionMode>(inst->GetSingleWordInOperand(1u));
      execution.flags.set(mode);

      switch (mode) {
        case SpvExecutionModeInvocations:
          execution.invocations = inst->GetSingleWordInOperand(2u);
          break;

        case SpvExecutionModeLocalSize:
          execution.workgroup_size.x = inst->GetSingleWordInOperand(2u);
          execution.workgroup_size.y = inst->GetSingleWordInOperand(3u);
          execution.workgroup_size.z = inst->GetSingleWordInOperand(4u);
          break;

        case SpvExecutionModeOutputVertices:
          execution.output_vertices = inst->GetSingleWordInOperand(2u);
          break;

        default:
          break;
      }
      break;
    }

    case SpvOpName: {
      uint32_t id = inst->GetSingleWordInOperand(0u);
      ir_->set_name(id, reinterpret_cast<const char *>(
                            inst->GetInOperand(1u).words.data()));
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

    case SpvOpTypeStruct: {
      uint32_t id = inst->result_id();
      auto &type = set<spirv_cross::SPIRType>(id);
      type.basetype = spirv_cross::SPIRType::Struct;
      for (uint32_t i = 0; i < inst->NumInOperands(); i++) {
        type.member_types.push_back(inst->GetSingleWordInOperand(i));
      }

      bool consider_aliasing = !ir_->get_name(type.self).empty();
      if (consider_aliasing) {
        for (auto &other : global_struct_cache) {
          if (ir_->get_name(type.self) == ir_->get_name(other) &&
              types_are_logically_equivalent(
                  type, get<spirv_cross::SPIRType>(other))) {
            type.type_alias = other;
            break;
          }
        }

        if (type.type_alias == spirv_cross::TypeID(0))
          global_struct_cache.push_back(id);
      }
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

    case SpvOpFunctionParameter: {
      uint32_t type = inst->type_id();
      uint32_t id = inst->result_id();

      assert(current_function_ && "Must be in a function!");

      current_function_->add_parameter(type, id);
      set<spirv_cross::SPIRVariable>(id, type, spv::StorageClassFunction);
      // TODO(sarahM0): figure out what did it do with arguments?

      break;
    }

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

    case SpvOpReturnValue: {
      assert(current_block_ && "Trying to end a non-existing block.");
      current_block_->terminator = spirv_cross::SPIRBlock::Return;
      current_block_->return_value = inst->GetSingleWordInOperand(0u);
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

    case SpvOpTypeSampledImage: {
      uint32_t id = inst->result_id();
      uint32_t imagetype = inst->GetSingleWordInOperand(0u);
      auto &type = set<spirv_cross::SPIRType>(id);
      type = get<spirv_cross::SPIRType>(imagetype);
      type.basetype = spirv_cross::SPIRType::SampledImage;
      type.self = id;
      break;
    }

    case SpvOpTypeSampler: {
      uint32_t id = inst->result_id();
      auto &type = set<spirv_cross::SPIRType>(id);
      type.basetype = spirv_cross::SPIRType::Sampler;
      break;
    }

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

      // Do NOT set ptrbase.self!
      break;
    }

    // Variable declaration
    // All variables are essentially pointers with a storage qualifier.
    case SpvOpVariable: {
      uint32_t type = inst->type_id();
      uint32_t id = inst->result_id();
      auto storage =
          static_cast<spv::StorageClass>(inst->GetSingleWordInOperand(0u));
      uint32_t initializer =
          inst->NumInOperands() == 2 ? inst->GetSingleWordInOperand(1u) : 0;

      if (storage == spv::StorageClassFunction) {
        assert(current_function_ && "No function currently in scope");
        current_function_->add_local_variable(id);
      }

      set<spirv_cross::SPIRVariable>(id, type, storage, initializer);

      // hlsl based shaders don't have those decorations. force them and then
      // reset when reading/writing images
      auto &ttype = get<spirv_cross::SPIRType>(type);
      if (ttype.basetype == spirv_cross::SPIRType::BaseType::Image) {
        ir_->set_decoration(id, spv::DecorationNonWritable);
        ir_->set_decoration(id, spv::DecorationNonReadable);
      }

      break;
    }

    // Branch instructions end blocks.
    case SpvOpBranch: {
      assert(current_block_ && "Trying to end a non-existing block.");

      uint32_t target = inst->GetSingleWordInOperand(0u);
      current_block_->terminator = spirv_cross::SPIRBlock::Direct;
      current_block_->next_block = target;
      current_block_ = nullptr;
      break;
    }

    case SpvOpMemberName: {
      uint32_t id = inst->GetSingleWordInOperand(0u);
      uint32_t member = inst->GetSingleWordInOperand(1u);
      ir_->set_member_name(
          id, member,
          reinterpret_cast<const char *>(inst->GetInOperand(2u).words.data()));
      break;
    }

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

    case SpvOpTypeFloat: {
      uint32_t id = inst->result_id();
      uint32_t width = inst->GetSingleWordInOperand(0u);
      auto &type = set<spirv_cross::SPIRType>(id);
      if (width == 64)
        type.basetype = spirv_cross::SPIRType::Double;
      else if (width == 32)
        type.basetype = spirv_cross::SPIRType::Float;
      else if (width == 16)
        type.basetype = spirv_cross::SPIRType::Half;
      else
        assert("Unrecognized bit-width of floating point type.");
      type.width = width;
      break;
    }

    case SpvOpTypeRuntimeArray: {
      uint32_t id = inst->result_id();

      auto &base = get<spirv_cross::SPIRType>(inst->GetSingleWordInOperand(0u));
      auto &arraybase = set<spirv_cross::SPIRType>(id);

      arraybase = base;
      arraybase.array.push_back(0);
      arraybase.array_size_literal.push_back(true);
      arraybase.parent_type = inst->GetSingleWordInOperand(0u);
      // Do NOT set arraybase.self!
      break;
    }

    case SpvOpMemberDecorate: {
      uint32_t id = inst->GetSingleWordInOperand(0u);
      uint32_t member = inst->GetSingleWordInOperand(1u);
      auto decoration =
          static_cast<spv::Decoration>(inst->GetSingleWordInOperand(2u));
      if (inst->NumInOperands() > 3)
        ir_->set_member_decoration(id, member, decoration,
                                   inst->GetSingleWordInOperand(3u));
      else
        ir_->set_member_decoration(id, member, decoration);
      break;
    }

      // Constants
    case SpvOpSpecConstant:
    case SpvOpConstant: {
      uint32_t id = inst->result_id();
      auto &type = get<spirv_cross::SPIRType>(inst->type_id());

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

    // Build composite types by "inheriting".
    // NOTE: The self member is also copied! For pointers and array modifiers
    // this is a good thing since we can refer to decorations on pointee classes
    // which is needed for UBO/SSBO, I/O blocks in geometry/tess etc.
    case SpvOpTypeVector: {
      uint32_t id = inst->result_id();
      uint32_t vecsize = inst->GetSingleWordInOperand(1u);

      auto &base = get<spirv_cross::SPIRType>(inst->GetSingleWordInOperand(0u));
      auto &vecbase = set<spirv_cross::SPIRType>(id);

      vecbase = base;
      vecbase.vecsize = vecsize;
      vecbase.self = id;
      vecbase.parent_type = inst->GetSingleWordInOperand(0u);
      break;
    }

    case SpvOpTypeMatrix: {
      uint32_t id = inst->result_id();
      uint32_t vecsize = inst->GetSingleWordInOperand(1u);

      auto &base = get<spirv_cross::SPIRType>(inst->GetSingleWordInOperand(0u));
      auto &vecbase = set<spirv_cross::SPIRType>(id);

      vecbase = base;
      vecbase.vecsize = vecsize;
      vecbase.self = id;
      vecbase.parent_type = inst->GetSingleWordInOperand(0u);
      break;
    }

    case SpvOpSpecConstantComposite:
    case SpvOpConstantComposite: {
      uint32_t id = inst->result_id();
      uint32_t type = inst->type_id();

      auto &ctype = get<spirv_cross::SPIRType>(type);

      // We can have constants which are structs and arrays.
      // In this case, our SPIRConstant will be a list of other
      // SPIRConstant
      // ids which we can refer to.
      if (ctype.basetype == spirv_cross::SPIRType::Struct ||
          !ctype.array.empty()) {
        set<spirv_cross::SPIRConstant>(
            id, type, inst->GetInOperand(0u).words.data(),
            inst->NumInOperands(),
            inst->opcode() == SpvOpSpecConstantComposite);
      } else {
        assert(inst->NumInOperands() < 5 &&
               "OpConstantComposite only supports 1, 2, 3 and 4 elements.");

        spirv_cross::SPIRConstant remapped_constant_ops[4];
        const spirv_cross::SPIRConstant *c[4];
        for (uint32_t i = 1; i < inst->NumInOperands(); i++) {
          // Specialization constants operations can also be part of this.
          // We do not know their value, so any attempt to query
          // SPIRConstant
          // later will fail. We can only propagate the ID of the expression
          // and use to_expression on it.
          auto *constant_op = maybe_get<spirv_cross::SPIRConstantOp>(
              inst->GetSingleWordInOperand(i));
          auto *undef_op = maybe_get<spirv_cross::SPIRUndef>(
              inst->GetSingleWordInOperand(i));
          if (constant_op) {
            assert(inst->opcode() != SpvOpConstantComposite &&
                   "Specialization constant operation used in "
                   "OpConstantComposite.");

            remapped_constant_ops[i].make_null(
                get<spirv_cross::SPIRType>(constant_op->basetype));
            remapped_constant_ops[i].self = constant_op->self;
            remapped_constant_ops[i].constant_type = constant_op->basetype;
            remapped_constant_ops[i].specialization = true;
            c[i] = &remapped_constant_ops[i];
          } else if (undef_op) {
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

    case SpvOpDecorate:
    case SpvOpDecorateId: {
      // OpDecorateId technically supports an array of arguments, but our only
      // supported decorations are single uint, so merge decorate and
      // decorate-id here.
      uint32_t id = inst->GetSingleWordInOperand(0u);

      auto decoration =
          static_cast<spv::Decoration>(inst->GetSingleWordInOperand(1u));
      if (inst->NumInOperands() > 2) {
        ir_->meta[id].decoration_word_offset[decoration] =
            uint32_t(inst->GetInOperand(2u).words.data() - ir_->spirv.data());
        ir_->set_decoration(id, decoration, inst->GetSingleWordInOperand(2u));
      } else
        ir_->set_decoration(id, decoration);

      break;
    }

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
      type.image.access = (inst->NumInOperands() >= 8)
                              ? static_cast<spv::AccessQualifier>(
                                    inst->GetSingleWordInOperand(7u))
                              : spv::AccessQualifierMax;
      break;
    }

    // TODO(sarahM0): These opcodes are processed in the generator. Investigate
    // if we need to rewrite those functions as well.
    case SpvOpImageSampleImplicitLod:
    case SpvOpCompositeConstruct:
    case SpvOpFAdd:
    case SpvOpFMul:
    case SpvOpConvertFToS:
    case SpvOpSampledImage:
    case SpvOpVectorShuffle:
    case SpvOpFunctionCall:
    case SpvOpCompositeExtract:
    case SpvOpImageFetch:
    case SpvOpStore:
    case SpvOpAccessChain:
    case SpvOpArrayLength:
    case SpvOpBitcast:
    case SpvOpLoad: {
      assert(current_block_ && "Currently no block to insert opcode.");
      spirv_cross::Instruction instr = {};
      instr.op = inst->opcode();
      instr.count = inst->NumOperandWords() + 1;
      instr.offset = offset_ + 1;
      instr.length = instr.count - 1;

      assert(
          instr.count != 0 &&
          "SPIR-V instructions cannot consume 0 words. Invalid SPIR-V file.");
      assert(offset_ <= ir_->spirv.size() &&
             "SPIR-V instruction goes out of bounds.");
      current_block_->ops.push_back(instr);
      break;
    }
    default: {
      printf("Instruction %d not supported in spvc parser yet.\n",
             inst->opcode());
      break;
    }
  }

  offset_ += inst->NumOperandWords() + 1;
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

}  // namespace opt
}  // namespace spvtools
