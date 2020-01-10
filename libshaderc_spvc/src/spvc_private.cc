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

#include "spvc_private.h"

#include "spirv-tools/optimizer.hpp"
#include "spvcir_pass.h"

// Originally from libshaderc_utils/exceptions.h, copied here to avoid
// needing to depend on libshaderc_utils and pull in its dependency on
// glslang.
#if (defined(_MSC_VER) && !defined(_CPPUNWIND)) || !defined(__EXCEPTIONS)
#define TRY_IF_EXCEPTIONS_ENABLED
#define CATCH_IF_EXCEPTIONS_ENABLED(X) if (0)
#else
#define TRY_IF_EXCEPTIONS_ENABLED try
#define CATCH_IF_EXCEPTIONS_ENABLED(X) catch (X)
#endif

namespace spvc_private {

spv_target_env get_spv_target_env(shaderc_target_env env,
                                  shaderc_env_version version) {
  switch (env) {
    case shaderc_target_env_opengl:
    case shaderc_target_env_opengl_compat:
      switch (version) {
        case shaderc_env_version_opengl_4_5:
          return SPV_ENV_OPENGL_4_5;
        default:
          break;
      }
      break;
    case shaderc_target_env_vulkan:
      switch (version) {
        case shaderc_env_version_vulkan_1_0:
          return SPV_ENV_VULKAN_1_0;
        case shaderc_env_version_vulkan_1_1:
          return SPV_ENV_VULKAN_1_1;
        default:
          break;
      }

      break;
    case shaderc_target_env_webgpu:
      return SPV_ENV_WEBGPU_0;
    default:
      break;
  }
  return SPV_ENV_VULKAN_1_0;
}

void consume_spirv_tools_message(shaderc_spvc_context* context,
                                 spv_message_level_t level, const char* src,
                                 const spv_position_t& pos,
                                 const char* message) {
  context->messages.push_back(message);
}

shaderc_spvc_status validate_spirv(shaderc_spvc_context* context,
                                   spv_target_env env, const uint32_t* source,
                                   size_t source_len) {
  spvtools::SpirvTools tools(env);
  if (!tools.IsValid()) {
    context->messages.push_back("Could not initialize SPIRV-Tools.");
    return shaderc_spvc_status_internal_error;
  }

  tools.SetMessageConsumer(std::bind(
      consume_spirv_tools_message, context, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

  if (!tools.Validate(source, source_len, spvtools::ValidatorOptions())) {
    context->messages.push_back("Validation of shader failed.");
    return shaderc_spvc_status_validation_error;
  }

  return shaderc_spvc_status_success;
}

shaderc_spvc_status translate_spirv(shaderc_spvc_context* context,
                                    spv_target_env source_env,
                                    spv_target_env target_env,
                                    const uint32_t* source, size_t source_len,
                                    shaderc_spvc_compile_options_t options,
                                    std::vector<uint32_t>* target) {
  if (!target) {
    context->messages.push_back("null provided for translation destination.");
    return shaderc_spvc_status_transformation_error;
  }

  if (source_env == target_env) {
    target->resize(source_len);
    memcpy(target->data(), source, source_len * sizeof(uint32_t));
    return shaderc_spvc_status_success;
  }

  spvtools::Optimizer opt(source_env);
  opt.SetMessageConsumer(std::bind(
      consume_spirv_tools_message, context, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

  if (source_env == SPV_ENV_WEBGPU_0 && target_env == SPV_ENV_VULKAN_1_1) {
    opt.RegisterWebGPUToVulkanPasses();
  } else if (source_env == SPV_ENV_VULKAN_1_1 &&
             target_env == SPV_ENV_WEBGPU_0) {
    opt.RegisterVulkanToWebGPUPasses();
  } else {
    context->messages.push_back(
        "No defined transformation between source and "
        "target execution environments.");
    return shaderc_spvc_status_transformation_error;
  }

  if (options->robust_buffer_access_pass) {
    opt.RegisterPass(spvtools::CreateGraphicsRobustAccessPass());
  }

  if (!opt.Run(source, source_len, target)) {
    context->messages.push_back(
        "Transformations between source and target "
        "execution environments failed.");
    return shaderc_spvc_status_transformation_error;
  }

  return shaderc_spvc_status_success;
}

shaderc_spvc_status validate_and_translate_spirv(
    shaderc_spvc_context* context, const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options, std::vector<uint32_t>* target) {
  shaderc_spvc_status status;
  if (options->validate) {
    status = validate_spirv(context, options->source_env, source, source_len);
    if (status != shaderc_spvc_status_success) {
      context->messages.push_back("Validation of input source failed.");
      return status;
    }
  }

  status = translate_spirv(context, options->source_env, options->target_env,
                           source, source_len, options, target);
  if (status != shaderc_spvc_status_success) return status;

  if (options->validate && (options->source_env != options->target_env)) {
    // Re-run validation on input if actually transformed.
    status = validate_spirv(context, options->target_env, target->data(),
                            target->size());
    if (status != shaderc_spvc_status_success) {
      context->messages.push_back("Validation of transformed source failed.");
      return status;
    }
  }

  return status;
}

shaderc_spvc_status generate_shader(spirv_cross::Compiler* compiler,
                                    shaderc_spvc_compilation_result_t result) {
  TRY_IF_EXCEPTIONS_ENABLED {
    result->string_output = compiler->compile();
    // An exception during compiling would crash (if exceptions off) or jump to
    // the catch block (if exceptions on) so if we're here we know the compile
    // worked.
    return shaderc_spvc_status_success;
  }
  CATCH_IF_EXCEPTIONS_ENABLED(...) {
    return shaderc_spvc_status_compilation_error;
  }
}

shaderc_spvc_status generate_glsl_compiler(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {
  spirv_cross::CompilerGLSL* cross_compiler;
  // spvc IR generation is under development, for now run spirv-cross
  // compiler, unless explicitly requested.
  // TODO (sarahM0): change the default to spvc IR generation when it's done
  if (context->use_spvc_parser) {
    shaderc_spvc_status status;
    spirv_cross::ParsedIR ir;
    status = generate_spvcir(context, &ir, source, source_len, options);
    if (status != shaderc_spvc_status_success) {
      context->messages.push_back(
          "Transformations between source and target "
          "execution environments failed (spvc-ir-pass).");
      return status;
    } else {
      cross_compiler = new (std::nothrow) spirv_cross::CompilerGLSL(ir);
    }
  } else {
    cross_compiler =
        new (std::nothrow) spirv_cross::CompilerGLSL(source, source_len);
  }

  if (!cross_compiler) {
    context->messages.push_back(
        "Unable to initialize SPIRV-Cross GLSL compiler.");
    return shaderc_spvc_status_compilation_error;
  }
  context->cross_compiler.reset(cross_compiler);

  if (options->glsl.version == 0) {
    // no version requested, was one detected in source?
    options->glsl.version = cross_compiler->get_common_options().version;
    if (options->glsl.version == 0) {
      // no version detected in source, use default
      options->glsl.version = DEFAULT_GLSL_VERSION;
    } else {
      // version detected implies ES also detected
      options->glsl.es = cross_compiler->get_common_options().es;
    }
  }

  // Override detected setting, if any.
  if (options->force_es) options->glsl.es = options->forced_es_setting;

  auto entry_points = cross_compiler->get_entry_points_and_stages();
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
      context->cross_compiler.reset();
      if (stage_count == 0) {
        context->messages.push_back("There is no entry point with name: " +
                                    options->entry_point);
      } else {
        context->messages.push_back(
            "There is more than one entry point with name: " +
            options->entry_point + ". Use --stage.");
      }
      return shaderc_spvc_status_compilation_error;
    }
  }

  if (!options->entry_point.empty()) {
    cross_compiler->set_entry_point(options->entry_point, model);
  }

  if (!options->glsl.vulkan_semantics) {
    uint32_t sampler =
        cross_compiler->build_dummy_sampler_for_combined_images();
    if (sampler) {
      // Set some defaults to make validation happy.
      cross_compiler->set_decoration(sampler, spv::DecorationDescriptorSet, 0);
      cross_compiler->set_decoration(sampler, spv::DecorationBinding, 0);
    }
  }

  spirv_cross::ShaderResources res;
  if (options->remove_unused_variables) {
    auto active = cross_compiler->get_active_interface_variables();
    res = cross_compiler->get_shader_resources(active);
    cross_compiler->set_enabled_interface_variables(move(active));
  } else {
    res = cross_compiler->get_shader_resources();
  }

  if (options->flatten_ubo) {
    for (auto& ubo : res.uniform_buffers)
      cross_compiler->flatten_buffer_block(ubo.id);
    for (auto& ubo : res.push_constant_buffers)
      cross_compiler->flatten_buffer_block(ubo.id);
  }

  if (!options->glsl.vulkan_semantics) {
    cross_compiler->build_combined_image_samplers();

    // if (args.combined_samplers_inherit_bindings)
    //  spirv_cross_util::inherit_combined_sampler_bindings(*compiler);

    // Give the remapped combined samplers new names.
    for (auto& remap : cross_compiler->get_combined_image_samplers()) {
      cross_compiler->set_name(
          remap.combined_id,
          spirv_cross::join("SPIRV_Cross_Combined",
                            cross_compiler->get_name(remap.image_id),
                            cross_compiler->get_name(remap.sampler_id)));
    }
  }

  cross_compiler->set_common_options(options->glsl);

  return shaderc_spvc_status_success;
}

shaderc_spvc_status generate_hlsl_compiler(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {
  spirv_cross::CompilerHLSL* cross_compiler;
  // spvc IR generation is under development, for now run spirv-cross
  // compiler, unless explicitly requested.
  // TODO (sarahM0): change the default to spvc IR generation when it's done
  if (context->use_spvc_parser) {
    shaderc_spvc_status status;
    spirv_cross::ParsedIR ir;
    status = generate_spvcir(context, &ir, source, source_len, options);
    if (status != shaderc_spvc_status_success) {
      context->messages.push_back(
          "Transformations between source and target "
          "execution environments failed (spvc-ir-pass).");
      return status;
    } else {
      cross_compiler = new (std::nothrow) spirv_cross::CompilerHLSL(ir);
    }
  } else {
    cross_compiler =
        new (std::nothrow) spirv_cross::CompilerHLSL(source, source_len);
  }
  if (!cross_compiler) {
    context->messages.push_back(
        "Unable to initialize SPIRV-Cross HLSL compiler.");
    return shaderc_spvc_status_compilation_error;
  }
  context->cross_compiler.reset(cross_compiler);

  cross_compiler->set_common_options(options->glsl);
  cross_compiler->set_hlsl_options(options->hlsl);

  return shaderc_spvc_status_success;
}

shaderc_spvc_status generate_msl_compiler(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {
  spirv_cross::CompilerMSL* cross_compiler;
  // spvc IR generation is under development, for now run spirv-cross
  // compiler, unless explicitly requested.
  // TODO (sarahM0): change the default to spvc IR generation when it's done
  if (context->use_spvc_parser) {
    shaderc_spvc_status status;
    spirv_cross::ParsedIR ir;
    status = generate_spvcir(context, &ir, source, source_len, options);
    if (status != shaderc_spvc_status_success) {
      context->messages.push_back(
          "Transformations between source and target "
          "execution environments failed (spvc-ir-pass).");
      return status;
    } else {
      cross_compiler = new (std::nothrow) spirv_cross::CompilerMSL(ir);
    }
  } else {
    cross_compiler =
        new (std::nothrow) spirv_cross::CompilerMSL(source, source_len);
  }

  if (!cross_compiler) {
    context->messages.push_back(
        "Unable to initialize SPIRV-Cross MSL compiler.");
    return shaderc_spvc_status_compilation_error;
  }
  context->cross_compiler.reset(cross_compiler);

  cross_compiler->set_common_options(options->glsl);
  cross_compiler->set_msl_options(options->msl);
  for (auto i : options->msl_discrete_descriptor_sets)
    cross_compiler->add_discrete_descriptor_set(i);

  return shaderc_spvc_status_success;
}

shaderc_spvc_status generate_vulkan_compiler(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {
  spirv_cross::CompilerReflection* cross_compiler;

  // spvc IR generation is under development, for now run spirv-cross
  // compiler, unless explicitly requested.
  // TODO (sarahM0): change the default to spvc IR generation when it's done
  if (context->use_spvc_parser) {
    spirv_cross::ParsedIR ir;
    shaderc_spvc_status status =
        generate_spvcir(context, &ir, source, source_len, options);
    if (status != shaderc_spvc_status_success) {
      context->messages.push_back(
          "Transformations between source and target "
          "execution environments failed (spvc-ir-pass).");
      return status;
    } else {
      cross_compiler = new (std::nothrow) spirv_cross::CompilerReflection(ir);
    }
  } else {
    cross_compiler =
        new (std::nothrow) spirv_cross::CompilerReflection(source, source_len);
  }

  if (!cross_compiler) {
    context->messages.push_back(
        "Unable to initialize SPIRV-Cross reflection "
        "compiler.");
    return shaderc_spvc_status_compilation_error;
  }
  context->cross_compiler.reset(cross_compiler);

  return shaderc_spvc_status_success;
}

shaderc_spvc_status generate_spvcir(const shaderc_spvc_context_t context,
                                    spirv_cross::ParsedIR* ir,
                                    const uint32_t* source, size_t source_len,
                                    shaderc_spvc_compile_options_t options) {
  if (context->use_spvc_parser) {
    std::vector<uint32_t> binary_output;
    spvtools::Optimizer opt(options->source_env);
    opt.SetMessageConsumer(std::bind(
        consume_spirv_tools_message, context, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    ir->spirv = std::vector<uint32_t>(source, source + source_len);

    opt.RegisterPass(
        spvtools::Optimizer::PassToken(std::unique_ptr<spvtools::opt::Pass>(
            reinterpret_cast<spvtools::opt::Pass*>(
                new spvtools::opt::SpvcIrPass(ir)))));

    if (!opt.Run(source, source_len, &binary_output)) {
      return shaderc_spvc_status_transformation_error;
    }
  }
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_decoration_to_spirv_cross_decoration(
    const shaderc_spvc_decoration decoration,
    spv::Decoration* spirv_cross_decoration_ptr) {
  if (!spirv_cross_decoration_ptr) return shaderc_spvc_status_internal_error;

  shaderc_spvc_status status = shaderc_spvc_status_success;

  switch (decoration) {
    case shaderc_spvc_decoration_specid:
      *spirv_cross_decoration_ptr = spv::DecorationSpecId;
      break;
    case shaderc_spvc_decoration_block:
      *spirv_cross_decoration_ptr = spv::DecorationBlock;
      break;
    case shaderc_spvc_decoration_rowmajor:
      *spirv_cross_decoration_ptr = spv::DecorationRowMajor;
      break;
    case shaderc_spvc_decoration_colmajor:
      *spirv_cross_decoration_ptr = spv::DecorationColMajor;
      break;
    case shaderc_spvc_decoration_arraystride:
      *spirv_cross_decoration_ptr = spv::DecorationArrayStride;
      break;
    case shaderc_spvc_decoration_matrixstride:
      *spirv_cross_decoration_ptr = spv::DecorationMatrixStride;
      break;
    case shaderc_spvc_decoration_builtin:
      *spirv_cross_decoration_ptr = spv::DecorationBuiltIn;
      break;
    case shaderc_spvc_decoration_noperspective:
      *spirv_cross_decoration_ptr = spv::DecorationNoPerspective;
      break;
    case shaderc_spvc_decoration_flat:
      *spirv_cross_decoration_ptr = spv::DecorationFlat;
      break;
    case shaderc_spvc_decoration_centroid:
      *spirv_cross_decoration_ptr = spv::DecorationCentroid;
      break;
    case shaderc_spvc_decoration_restrict:
      *spirv_cross_decoration_ptr = spv::DecorationRestrict;
      break;
    case shaderc_spvc_decoration_aliased:
      *spirv_cross_decoration_ptr = spv::DecorationAliased;
      break;
    case shaderc_spvc_decoration_nonwritable:
      *spirv_cross_decoration_ptr = spv::DecorationNonWritable;
      break;
    case shaderc_spvc_decoration_nonreadable:
      *spirv_cross_decoration_ptr = spv::DecorationNonReadable;
      break;
    case shaderc_spvc_decoration_uniform:
      *spirv_cross_decoration_ptr = spv::DecorationUniform;
      break;
    case shaderc_spvc_decoration_location:
      *spirv_cross_decoration_ptr = spv::DecorationLocation;
      break;
    case shaderc_spvc_decoration_component:
      *spirv_cross_decoration_ptr = spv::DecorationComponent;
      break;
    case shaderc_spvc_decoration_index:
      *spirv_cross_decoration_ptr = spv::DecorationIndex;
      break;
    case shaderc_spvc_decoration_binding:
      *spirv_cross_decoration_ptr = spv::DecorationBinding;
      break;
    case shaderc_spvc_decoration_descriptorset:
      *spirv_cross_decoration_ptr = spv::DecorationDescriptorSet;
      break;
    case shaderc_spvc_decoration_offset:
      *spirv_cross_decoration_ptr = spv::DecorationOffset;
      break;
    case shaderc_spvc_decoration_nocontraction:
      *spirv_cross_decoration_ptr = spv::DecorationNoContraction;
      break;
    default:
      *spirv_cross_decoration_ptr = spv::DecorationMax;
      status = shaderc_spvc_status_internal_error;
      break;
  }
  return status;
}

}  // namespace spvc_private
