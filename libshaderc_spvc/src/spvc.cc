// Copyright 2018 The Shaderc Authors. All rights reserved.
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

#include "shaderc/spvc.h"
#include "libshaderc_util/exceptions.h"

#include "spirv-cross/spirv_cross_parsed_ir.hpp"
#include "spirv-cross/spirv_glsl.hpp"
#include "spirv-cross/spirv_hlsl.hpp"
#include "spirv-cross/spirv_msl.hpp"
#include "spirv-cross/spirv_parser.hpp"
#include "spirv-tools/libspirv.hpp"

// GLSL version produced when none specified nor detected from source.
#define DEFAULT_GLSL_VERSION 450

struct shaderc_spvc_compiler {};

// Described in spvc.h.
struct shaderc_spvc_compilation_result {
  std::string output;
  std::string messages;
  shaderc_compilation_status status =
      shaderc_compilation_status_null_result_object;
};

struct shaderc_spvc_compile_options {
  shaderc_spvc_parser parser = shaderc_spvc_parser_spirv_cross;
  bool validate = true;
  bool remove_unused_variables = false;
  bool flatten_ubo = false;
  std::string entry_point;
  spv_target_env target_env = SPV_ENV_VULKAN_1_0;
  spirv_cross::CompilerGLSL::Options glsl;
  spirv_cross::CompilerHLSL::Options hlsl;
  spirv_cross::CompilerMSL::Options msl;
};

shaderc_spvc_compile_options_t shaderc_spvc_compile_options_initialize() {
  shaderc_spvc_compile_options_t options =
      new (std::nothrow) shaderc_spvc_compile_options;
  if (options) {
    options->glsl.version = 0;
    options->hlsl.point_size_compat = true;
    options->hlsl.point_coord_compat = true;
  }
  return options;
}

shaderc_spvc_compile_options_t shaderc_spvc_compile_options_clone(
    shaderc_spvc_compile_options_t options) {
  if (options) return new (std::nothrow) shaderc_spvc_compile_options(*options);
  return shaderc_spvc_compile_options_initialize();
}

void shaderc_spvc_compile_options_release(
    shaderc_spvc_compile_options_t options) {
  delete options;
}

void shaderc_spvc_compile_options_set_parser(
    shaderc_spvc_compile_options_t options, shaderc_spvc_parser parser) {
  options->parser = parser;
}

void shaderc_spvc_compile_options_set_target_env(
    shaderc_spvc_compile_options_t options, shaderc_target_env target,
    shaderc_env_version version) {
  switch (target) {
    case shaderc_target_env_opengl:
    case shaderc_target_env_opengl_compat:
      switch (version) {
        case shaderc_env_version_opengl_4_5:
          options->target_env = SPV_ENV_OPENGL_4_5;
          break;
        default:
          break;
      }
      break;
    case shaderc_target_env_vulkan:
      switch (version) {
        case shaderc_env_version_vulkan_1_0:
          options->target_env = SPV_ENV_VULKAN_1_0;
          break;
        case shaderc_env_version_vulkan_1_1:
          options->target_env = SPV_ENV_VULKAN_1_1;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void shaderc_spvc_compile_options_set_entry_point(
    shaderc_spvc_compile_options_t options, const char* entry_point) {
  options->entry_point = entry_point;
}

void shaderc_spvc_compile_options_set_remove_unused_variables(
    shaderc_spvc_compile_options_t options, bool b) {
  options->remove_unused_variables = b;
}

void shaderc_spvc_compile_options_set_vulkan_semantics(
    shaderc_spvc_compile_options_t options, bool b) {
  options->glsl.vulkan_semantics = b;
}

void shaderc_spvc_compile_options_set_separate_shader_objects(
    shaderc_spvc_compile_options_t options, bool b) {
  options->glsl.separate_shader_objects = b;
}

void shaderc_spvc_compile_options_set_flatten_ubo(
    shaderc_spvc_compile_options_t options, bool b) {
  options->flatten_ubo = b;
}

void shaderc_spvc_compile_options_set_glsl_language_version(
    shaderc_spvc_compile_options_t options, uint32_t version) {
  options->glsl.version = version;
}

void shaderc_spvc_compile_options_set_msl_language_version(
    shaderc_spvc_compile_options_t options, uint32_t version) {
  options->msl.msl_version = version;
}

void shaderc_spvc_compile_options_set_shader_model(
    shaderc_spvc_compile_options_t options, uint32_t model) {
  options->hlsl.shader_model = model;
}

void shaderc_spvc_compile_options_set_fixup_clipspace(
    shaderc_spvc_compile_options_t options, bool b) {
  options->glsl.vertex.fixup_clipspace = b;
}

void shaderc_spvc_compile_options_set_flip_vert_y(
    shaderc_spvc_compile_options_t options, bool b) {
  options->glsl.vertex.flip_vert_y = b;
}

size_t shaderc_spvc_compile_options_set_for_fuzzing(
    shaderc_spvc_compile_options_t options, const uint8_t* data, size_t size) {
  if (!data || size < sizeof(*options)) return 0;

  memcpy(options, data, sizeof(*options));
  return sizeof(*options);
}

shaderc_spvc_compiler_t shaderc_spvc_compiler_initialize() {
  return new (std::nothrow) shaderc_spvc_compiler;
}

void shaderc_spvc_compiler_release(shaderc_spvc_compiler_t compiler) {
  delete compiler;
}

namespace {

// Put the index-th operand of the instruction into 'out'
template <typename Out>
void GetOp(const spv_parsed_instruction_t* inst, unsigned index, Out& out) {
  static_assert(sizeof(Out) == sizeof(uint32_t), "unexpected operand size");
  assert(index < inst->num_operands);
  out = static_cast<Out>(inst->words[inst->operands[index].offset]);
}

// Specialized for string.
template <>
void GetOp(const spv_parsed_instruction_t* inst, unsigned index,
           std::string& out) {
  assert(index < inst->num_operands);
  const spv_parsed_operand_t& op = inst->operands[index];
  assert(op.type == SPV_OPERAND_TYPE_LITERAL_STRING);
  for (const uint32_t* p = inst->words + op.offset;; ++p) {
    for (unsigned i = 0; i < 4; ++i) {
      unsigned char c = (*p >> (i * 8)) & 0xff;
      if (!c) return;
      out.push_back(c);
    }
  }
}

// Specialized for ID list.
template <>
void GetOp(const spv_parsed_instruction_t* inst, unsigned index,
           std::vector<uint32_t>& out) {
  if (index < inst->num_operands) {
    const spv_parsed_operand_t& op = inst->operands[index];
    assert(op.type == SPV_OPERAND_TYPE_ID);
    const uint32_t* p = inst->words + op.offset;
    std::copy(p, p + inst->num_operands - index, back_inserter(out));
  }
}

// Recursive template base case.
void GetOpsRecur(const spv_parsed_instruction_t* inst, unsigned index) {}

// Recursive template to do any number of GetOp().
template <typename First, typename... Rest>
void GetOpsRecur(const spv_parsed_instruction_t* inst, unsigned index,
                 First& first, Rest&... rest) {
  GetOp(inst, index, first);
  GetOpsRecur(inst, index + 1, rest...);
}

// Start the recursive template.
template <typename... Outs>
void GetOps(const spv_parsed_instruction_t* inst, Outs&... outs) {
  GetOpsRecur(inst, 0, outs...);
}

// This struct holds a SPIRV-Cross compiler object of type 'CompilerType,'
// all the state needed while building the spirv_cross::ParsedIR object consumed
// by the compiler, and a shaderc_spvc_compilation_result_t which indicates
// the result of validation/parsing/compiling.
template <class CompilerType>
struct Compiler {
  CompilerType* cross = nullptr;
  spirv_cross::SPIRFunction* current_function = nullptr;
  spirv_cross::SPIRBlock* current_block = nullptr;
  spirv_cross::ParsedIR ir;
  shaderc_spvc_compilation_result_t result =
      nullptr;  // NOT freed by our destructor

  void consume_validation_message(spv_message_level_t level, const char* src,
                                  const spv_position_t& pos,
                                  const char* message) {
    assert(result);
    result->messages.append(message);
    result->messages.append("\n");
  }

  static spv_result_t parse_header(void* user_data, spv_endianness_t endian,
                                   uint32_t magic, uint32_t version,
                                   uint32_t generator, uint32_t id_bound,
                                   uint32_t reserved) {
    return ((Compiler*)user_data)
        ->parse_header(endian, magic, version, generator, id_bound, reserved);
  }

  spv_result_t parse_header(spv_endianness_t endian, uint32_t magic,
                            uint32_t version, uint32_t generator,
                            uint32_t id_bound, uint32_t reserved) {
    ir.set_id_bounds(id_bound);
    return SPV_SUCCESS;
  }

  static spv_result_t parse_instruction(void* user_data,
                                        const spv_parsed_instruction_t* inst) {
    return ((Compiler*)user_data)->parse_instruction(inst);
  }

  // Duplicate behavior of SPIRV-Cross.
  spv_result_t parse_instruction(const spv_parsed_instruction_t* inst) {
    switch (inst->opcode) {
      case spv::OpExtInstImport: {
        uint32_t id;
        std::string ext;
        GetOps(inst, id, ext);
        if (ext == "GLSL.std.450")
          set<spirv_cross::SPIRExtension>(id, spirv_cross::SPIRExtension::GLSL);
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
      } break;

      case spv::OpEntryPoint: {
        spv::ExecutionModel model;
        uint32_t id;
        std::string name;
        std::vector<uint32_t> interface_variables;
        GetOps(inst, model, id, name, interface_variables);

        auto itr = ir.entry_points.insert(
            std::make_pair(id, spirv_cross::SPIREntryPoint(id, model, name)));
        auto& e = itr.first->second;
        std::copy(interface_variables.begin(), interface_variables.end(),
                  back_inserter(e.interface_variables));
        ir.set_name(id, e.name);
        if (!ir.default_entry_point) ir.default_entry_point = id;
      } break;

      case spv::OpExecutionMode: {
        uint32_t entry_point;
        spv::ExecutionMode mode;
        GetOps(inst, entry_point, mode);

        auto& execution = ir.entry_points[entry_point];
        execution.flags.set(mode);

        switch (mode) {
          case spv::ExecutionModeInvocations:
            GetOpsRecur(inst, 2, execution.invocations);
            break;
          case spv::ExecutionModeLocalSize:
            GetOpsRecur(inst, 2, execution.workgroup_size.x,
                        execution.workgroup_size.y, execution.workgroup_size.z);
            break;
          case spv::ExecutionModeOutputVertices:
            GetOpsRecur(inst, 2, execution.output_vertices);
            break;
          default:
            break;
        }
      } break;

      case spv::OpCapability: {
        spv::Capability cap;
        GetOps(inst, cap);
        if (cap == spv::CapabilityKernel) {
          result->messages.append("Kernel capability not supported.");
          return SPV_REQUESTED_TERMINATION;
        }
        ir.declared_capabilities.push_back(cap);
      } break;

      case spv::OpTypeVoid: {
        uint32_t id;
        GetOps(inst, id);
        auto& type = set<spirv_cross::SPIRType>(inst->result_id);
        type.basetype = spirv_cross::SPIRType::Void;
      } break;

      case spv::OpTypeFunction: {
        uint32_t id;
        uint32_t ret;
        auto& func =
            set<spirv_cross::SPIRFunctionPrototype>(inst->result_id, ret);
        GetOps(inst, id, ret, func.parameter_types);
      } break;

      case spv::OpFunction: {
        uint32_t res, id, control, type;
        GetOps(inst, res, id, control, type);

        if (current_function) {
          result->messages.append(
              "Must end a function before starting a new one!");
          return SPV_REQUESTED_TERMINATION;
        }

        current_function =
            &set<spirv_cross::SPIRFunction>(inst->result_id, res, type);
      } break;

      case spv::OpFunctionEnd: {
        if (current_block) {
          result->messages.append(
              "Cannot end a function before ending the current block.\nLikely "
              "cause: If this SPIR-V was created from glslang HLSL, make sure "
              "the entry point is valid.");
          return SPV_REQUESTED_TERMINATION;
        }
        current_function = nullptr;
      } break;

      case spv::OpLabel: {
        if (!current_function) {
          result->messages.append("Blocks cannot exist outside functions!");
          return SPV_REQUESTED_TERMINATION;
        }
        uint32_t id;
        GetOps(inst, id);

        current_function->blocks.push_back(inst->result_id);
        if (!current_function->entry_block) {
          current_function->entry_block = inst->result_id;
        }

        if (current_block) {
          result->messages.append(
              "Cannot start a block before ending the current block.");
          return SPV_REQUESTED_TERMINATION;
        }

        current_block = &set<spirv_cross::SPIRBlock>(inst->result_id);
      } break;

      case spv::OpReturn: {
        if (!current_block) {
          result->messages.append("Trying to end a non-existing block.");
          return SPV_REQUESTED_TERMINATION;
        }
        current_block->terminator = spirv_cross::SPIRBlock::Return;
        current_block = nullptr;
      } break;

      case spv::OpMemoryModel:
        // Don't need to do anything.
        break;

      default:
        std::ostringstream msg;
        msg << "Instruction " << inst->opcode << " not supported yet.";
        result->messages.append(msg.str());
        return SPV_REQUESTED_TERMINATION;
    }

    return SPV_SUCCESS;
  }

  // Copied from SPIRV-Cross.
  template <typename T, typename... P>
  T& set(uint32_t id, P&&... args) {
    ir.add_typed_id(static_cast<spirv_cross::Types>(T::type), id);
    auto& v = spirv_cross::variant_set<T>(ir.ids[id], std::forward<P>(args)...);
    v.self = id;
    return v;
  }

  bool ok() {
    return result && result->status == shaderc_compilation_status_success;
  }

  ~Compiler() { delete cross; }

  // Validate the source spir-v if requested.  If valid parse with either the
  // SPIRV-Cross parser or the SPIRV-Tools parser then use the result to
  // construct a SPIRV-Cross compiler.
  Compiler(const uint32_t* source, size_t source_len,
           shaderc_spvc_compile_options_t options) {
    result = new (std::nothrow) shaderc_spvc_compilation_result;
    if (!result) return;

    if (options->validate) {
      spvtools::SpirvTools tools(options->target_env);
      if (!tools.IsValid()) {
        result->messages.append(
            "could not set up SpirvTools - bad target env?");
        result->status = shaderc_compilation_status_internal_error;
        return;
      }
      tools.SetMessageConsumer(std::bind(
          &Compiler::consume_validation_message, this, std::placeholders::_1,
          std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
      if (!tools.Validate(source, source_len, spvtools::ValidatorOptions())) {
        result->status = shaderc_compilation_status_validation_error;
        return;
      }
    }

    if (options->parser == shaderc_spvc_parser_spirv_tools) {
      ir.spirv = std::vector<uint32_t>(source, source + source_len);
      spv_context context = spvContextCreate(options->target_env);
      spv_result_t parse_result =
          spvBinaryParse(context, this, source, source_len, parse_header,
                         parse_instruction, nullptr);
      spvContextDestroy(context);
      if (parse_result != SPV_SUCCESS) {
        result->messages.append("Failed to parse SPIR-V.");
        result->status = shaderc_compilation_status_internal_error;
        return;
      }
      cross = new (std::nothrow) CompilerType(ir);
    } else {
      spirv_cross::Parser parser(source, source_len);
      parser.parse();
      cross = new (std::nothrow) CompilerType(parser.get_parsed_ir());
    }

    if (cross) {
      result->status = shaderc_compilation_status_success;
    } else {
      result->status = shaderc_compilation_status_internal_error;
    }
  }

  void compile() {
    if (!ok()) return;
    assert(cross);
    TRY_IF_EXCEPTIONS_ENABLED {
      result->output = cross->compile();
      // An exception during compiling would crash (if exceptions off) or jump
      // to the catch block (if exceptions on) so if we're here we know the
      // compile worked.
      result->status = shaderc_compilation_status_success;
    }
    CATCH_IF_EXCEPTIONS_ENABLED(...) {
      result->status = shaderc_compilation_status_compilation_error;
      result->messages = "Compilation failed.  Partial source:";
      result->messages.append(cross->get_partial_source());
    }
  }
};

}  // namespace

shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_glsl(
    const shaderc_spvc_compiler_t, const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options) {
  Compiler<spirv_cross::CompilerGLSL> compiler(source, source_len, options);
  if (!compiler.ok()) return compiler.result;

  if (options->glsl.version == 0) {
    // no version requested, was one detected in source?
    options->glsl.version = compiler.cross->get_common_options().version;
    if (options->glsl.version == 0) {
      // no version detected in source, use default
      options->glsl.version = DEFAULT_GLSL_VERSION;
    } else {
      // version detected implies ES also detected
      options->glsl.es = compiler.cross->get_common_options().es;
    }
  }

  auto entry_points = compiler.cross->get_entry_points_and_stages();
  spv::ExecutionModel model = spv::ExecutionModelMax;
  if (!options->entry_point.empty()) {
    // Make sure there is just one entry point with this name, or the stage is
    // ambiguous.
    uint32_t stage_count = 0;
    for (auto& e : entry_points) {
      if (e.name == options->entry_point) {
        stage_count++;
        model = e.execution_model;
      }
    }
    if (stage_count != 1) {
      compiler.result->status = shaderc_compilation_status_compilation_error;
      if (stage_count == 0)
        compiler.result->messages =
            "There is no entry point with name: " + options->entry_point;
      else
        compiler.result->messages =
            "There is more than one entry point with name: " +
            options->entry_point + ". Use --stage.";
      return compiler.result;
    }
  }
  if (!options->entry_point.empty()) {
    compiler.cross->set_entry_point(options->entry_point, model);
  }

  if (!options->glsl.vulkan_semantics) {
    uint32_t sampler =
        compiler.cross->build_dummy_sampler_for_combined_images();
    if (sampler) {
      // Set some defaults to make validation happy.
      compiler.cross->set_decoration(sampler, spv::DecorationDescriptorSet, 0);
      compiler.cross->set_decoration(sampler, spv::DecorationBinding, 0);
    }
  }

  spirv_cross::ShaderResources res;
  if (options->remove_unused_variables) {
    auto active = compiler.cross->get_active_interface_variables();
    res = compiler.cross->get_shader_resources(active);
    compiler.cross->set_enabled_interface_variables(move(active));
  } else {
    res = compiler.cross->get_shader_resources();
  }

  if (options->flatten_ubo) {
    for (auto& ubo : res.uniform_buffers)
      compiler.cross->flatten_buffer_block(ubo.id);
    for (auto& ubo : res.push_constant_buffers)
      compiler.cross->flatten_buffer_block(ubo.id);
  }

  if (!options->glsl.vulkan_semantics) {
    compiler.cross->build_combined_image_samplers();

    // Give the remapped combined samplers new names.
    for (auto& remap : compiler.cross->get_combined_image_samplers()) {
      compiler.cross->set_name(
          remap.combined_id,
          spirv_cross::join("SPIRV_Cross_Combined",
                            compiler.cross->get_name(remap.image_id),
                            compiler.cross->get_name(remap.sampler_id)));
    }
  }

  compiler.cross->set_common_options(options->glsl);
  compiler.compile();
  return compiler.result;
}

shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_hlsl(
    const shaderc_spvc_compiler_t, const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options) {
  Compiler<spirv_cross::CompilerHLSL> compiler(source, source_len, options);
  if (!compiler.ok()) return compiler.result;
  compiler.cross->set_common_options(options->glsl);
  compiler.cross->set_hlsl_options(options->hlsl);
  compiler.compile();
  return compiler.result;
}

shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_msl(
    const shaderc_spvc_compiler_t, const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options) {
  Compiler<spirv_cross::CompilerMSL> compiler(source, source_len, options);
  if (!compiler.ok()) return compiler.result;
  compiler.cross->set_common_options(options->glsl);
  compiler.compile();
  return compiler.result;
}

const char* shaderc_spvc_result_get_output(
    const shaderc_spvc_compilation_result_t result) {
  return result->output.c_str();
}

const char* shaderc_spvc_result_get_messages(
    const shaderc_spvc_compilation_result_t result) {
  return result->messages.c_str();
}

shaderc_compilation_status shaderc_spvc_result_get_status(
    const shaderc_spvc_compilation_result_t result) {
  return result->status;
}

void shaderc_spvc_result_release(shaderc_spvc_compilation_result_t result) {
  delete result;
}
