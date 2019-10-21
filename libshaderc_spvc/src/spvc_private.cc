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

#include "libshaderc_util/exceptions.h"
#include "spirv-tools/optimizer.hpp"
#if SHADERC_ENABLE_SPVC_PARSER
#include "spvcir_pass.h"
#endif  // SHADERC_ENABLE_SPVC_PARSER

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

void consume_spirv_tools_message(shaderc_spvc_compilation_result* result,
                                 spv_message_level_t level, const char* src,
                                 const spv_position_t& pos,
                                 const char* message) {
  result->messages.append(message);
  result->messages.append("\n");
}

shaderc_spvc_compilation_result_t validate_spirv(
    spv_target_env env, const uint32_t* source, size_t source_len,
    shaderc_spvc_compilation_result_t result) {
  spvtools::SpirvTools tools(env);
  if (!tools.IsValid()) {
    result->messages.append("Could not initialize SPIRV-Tools.\n");
    result->status = shaderc_compilation_status_internal_error;
    return result;
  }

  tools.SetMessageConsumer(std::bind(
      consume_spirv_tools_message, result, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

  if (!tools.Validate(source, source_len, spvtools::ValidatorOptions())) {
    result->messages.append("Validation of shader failed.\n");
    result->status = shaderc_compilation_status_validation_error;
    return result;
  }
  return result;
}

shaderc_spvc_compilation_result_t translate_spirv(
    spv_target_env source_env, spv_target_env target_env,
    const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options, std::vector<uint32_t>* target,
    shaderc_spvc_compilation_result_t result) {
  if (!target) {
    result->messages.append("null provided for translation destination.\n");
    result->status = shaderc_compilation_status_transformation_error;
    return result;
  }

  if (source_env == target_env) {
    target->resize(source_len);
    memcpy(target->data(), source, source_len * sizeof(uint32_t));
    return result;
  }

  spvtools::Optimizer opt(source_env);
  opt.SetMessageConsumer(std::bind(
      consume_spirv_tools_message, result, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

  if (source_env == SPV_ENV_WEBGPU_0 && target_env == SPV_ENV_VULKAN_1_1) {
    opt.RegisterWebGPUToVulkanPasses();
  } else if (source_env == SPV_ENV_VULKAN_1_1 &&
             target_env == SPV_ENV_WEBGPU_0) {
    opt.RegisterVulkanToWebGPUPasses();
  } else {
    result->messages.append(
        "No defined transformation between source and "
        "target execution environments.\n");
    result->status = shaderc_compilation_status_transformation_error;
    return result;
  }

  if (options->robust_buffer_access_pass) {
    opt.RegisterPass(spvtools::CreateGraphicsRobustAccessPass());
  }

  if (!opt.Run(source, source_len, target)) {
    result->messages.append(
        "Transformations between source and target "
        "execution environments failed.\n");
    result->status = shaderc_compilation_status_transformation_error;
    return result;
  }

  return result;
}

shaderc_spvc_compilation_result_t validate_and_translate_spirv(
    const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options, std::vector<uint32_t>* target,
    shaderc_spvc_compilation_result_t result) {
  if (options->validate) {
    result = validate_spirv(options->source_env, source, source_len, result);
    if (result->status != shaderc_compilation_status_success) {
      result->messages.append("Validation of input source failed.\n");
      return result;
    }
  }

  result = translate_spirv(options->source_env, options->target_env, source,
                           source_len, options, target, result);
  if (result->status != shaderc_compilation_status_success) return result;
  if (options->validate && (options->source_env != options->target_env)) {
    // Re-run validation on input if actually transformed.
    result = validate_spirv(options->target_env, target->data(), target->size(),
                            result);
    if (result->status != shaderc_compilation_status_success) {
      result->messages.append("Validation of transformed source failed.\n");
      return result;
    }
  }

  return result;
}

shaderc_spvc_compilation_result_t generate_shader(
    spirv_cross::Compiler* compiler, shaderc_spvc_compilation_result_t result) {
  TRY_IF_EXCEPTIONS_ENABLED {
    result->string_output = compiler->compile();
    // An exception during compiling would crash (if exceptions off) or jump to
    // the catch block (if exceptions on) so if we're here we know the compile
    // worked.
    result->status = shaderc_compilation_status_success;
  }
  CATCH_IF_EXCEPTIONS_ENABLED(...) {
    result->status = shaderc_compilation_status_compilation_error;
  }
  return result;
}

shaderc_spvc_compilation_result_t generate_glsl_shader(
    const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options,
    shaderc_spvc_compilation_result_t result) {
  spirv_cross::CompilerGLSL* compiler;

// spvc IR generation is under development, for now run spirv-cross
// compiler(SHADERC_ENABLE_SPVC_PARSER is OFF by default)
// TODO (sarahM0): change the default to spvc IR generation when it's done
#if SHADERC_ENABLE_SPVC_PARSER
  spirv_cross::ParsedIR ir;
  result = generate_spvcir(&ir, source, source_len, options, result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append(
        "Transformations between source and target "
        "execution environments failed (spvc-ir-pass).\n");
    return result;
  } else {
    compiler = new (std::nothrow) spirv_cross::CompilerGLSL(ir);
  }
#else
  compiler = new (std::nothrow) spirv_cross::CompilerGLSL(source, source_len);
#endif

  if (!compiler) {
    result->messages.append(
        "Unable to initialize SPIRV-Cross GLSL compiler.\n");
    result->status = shaderc_compilation_status_compilation_error;
    return result;
  }
  result->compiler.reset(compiler);

  if (options->glsl.version == 0) {
    // no version requested, was one detected in source?
    options->glsl.version = compiler->get_common_options().version;
    if (options->glsl.version == 0) {
      // no version detected in source, use default
      options->glsl.version = DEFAULT_GLSL_VERSION;
    } else {
      // version detected implies ES also detected
      options->glsl.es = compiler->get_common_options().es;
    }
  }

  // Override detected setting, if any.
  if (options->force_es) options->glsl.es = options->forced_es_setting;

  auto entry_points = compiler->get_entry_points_and_stages();
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
      result->status = shaderc_compilation_status_compilation_error;
      result->compiler.reset();
      if (stage_count == 0) {
        result->messages.append("There is no entry point with name: " +
                                options->entry_point);
      } else {
        result->messages.append(
            "There is more than one entry point with name: " +
            options->entry_point + ". Use --stage.");
      }
      return result;
    }
  }

  if (!options->entry_point.empty()) {
    compiler->set_entry_point(options->entry_point, model);
  }

  if (!options->glsl.vulkan_semantics) {
    uint32_t sampler = compiler->build_dummy_sampler_for_combined_images();
    if (sampler) {
      // Set some defaults to make validation happy.
      compiler->set_decoration(sampler, spv::DecorationDescriptorSet, 0);
      compiler->set_decoration(sampler, spv::DecorationBinding, 0);
    }
  }

  spirv_cross::ShaderResources res;
  if (options->remove_unused_variables) {
    auto active = compiler->get_active_interface_variables();
    res = compiler->get_shader_resources(active);
    compiler->set_enabled_interface_variables(move(active));
  } else {
    res = compiler->get_shader_resources();
  }

  if (options->flatten_ubo) {
    for (auto& ubo : res.uniform_buffers)
      compiler->flatten_buffer_block(ubo.id);
    for (auto& ubo : res.push_constant_buffers)
      compiler->flatten_buffer_block(ubo.id);
  }

  if (!options->glsl.vulkan_semantics) {
    compiler->build_combined_image_samplers();

    // if (args.combined_samplers_inherit_bindings)
    //  spirv_cross_util::inherit_combined_sampler_bindings(*compiler);

    // Give the remapped combined samplers new names.
    for (auto& remap : compiler->get_combined_image_samplers()) {
      compiler->set_name(
          remap.combined_id,
          spirv_cross::join("SPIRV_Cross_Combined",
                            compiler->get_name(remap.image_id),
                            compiler->get_name(remap.sampler_id)));
    }
  }

  compiler->set_common_options(options->glsl);

  result = generate_shader(compiler, result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append("Compilation failed.  Partial source:\n");
    result->messages.append(compiler->get_partial_source());
    result->compiler.reset();
  }

  return result;
}

shaderc_spvc_compilation_result_t generate_hlsl_shader(
    const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options,
    shaderc_spvc_compilation_result_t result) {
  spirv_cross::CompilerHLSL* compiler;

// spvc IR generation is under development, for now run spirv-cross
// compiler(SHADERC_ENABLE_SPVC_PARSER is OFF by default)
// TODO (sarahM0): change the default to spvc IR generation when it's done
#if SHADERC_ENABLE_SPVC_PARSER
  spirv_cross::ParsedIR ir;
  result = generate_spvcir(&ir, source, source_len, options, result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append(
        "Transformations between source and target "
        "execution environments failed (spvc-ir-pass).\n");
    return result;
  } else {
    compiler = new (std::nothrow) spirv_cross::CompilerHLSL(ir);
  }
#else
  compiler = new (std::nothrow) spirv_cross::CompilerHLSL(source, source_len);
#endif

  if (!compiler) {
    result->messages.append(
        "Unable to initialize SPIRV-Cross HLSL compiler.\n");
    result->status = shaderc_compilation_status_compilation_error;
    return result;
  }
  result->compiler.reset(compiler);

  compiler->set_common_options(options->glsl);
  compiler->set_hlsl_options(options->hlsl);

  result = generate_shader(compiler, result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append("Compilation failed.  Partial source:\n");
    result->messages.append(compiler->get_partial_source());
    result->compiler.reset();
  }

  return result;
}

shaderc_spvc_compilation_result_t generate_msl_shader(
    const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options,
    shaderc_spvc_compilation_result_t result) {
  spirv_cross::CompilerMSL* compiler;

// spvc IR generation is under development, for now run spirv-cross
// compiler(SHADERC_ENABLE_SPVC_PARSER is OFF by default)
// TODO (sarahM0): change the default to spvc IR generation when it's done
#if SHADERC_ENABLE_SPVC_PARSER
  spirv_cross::ParsedIR ir;
  result = generate_spvcir(&ir, source, source_len, options, result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append(
        "Transformations between source and target "
        "execution environments failed (spvc-ir-pass).\n");
    return result;
  } else {
    compiler = new (std::nothrow) spirv_cross::CompilerMSL(ir);
  }
#else
  compiler = new (std::nothrow) spirv_cross::CompilerMSL(source, source_len);
#endif

  if (!compiler) {
    result->messages.append("Unable to initialize SPIRV-Cross MSL compiler.\n");
    result->status = shaderc_compilation_status_compilation_error;
    return result;
  }
  result->compiler.reset(compiler);

  compiler->set_common_options(options->glsl);
  compiler->set_msl_options(options->msl);
  for (auto i : options->msl_discrete_descriptor_sets)
    compiler->add_discrete_descriptor_set(i);

  result = generate_shader(compiler, result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append("Compilation failed.  Partial source:\n");
    result->messages.append(compiler->get_partial_source());
    result->compiler.reset();
  }

  return result;
}  // namespace spvc_private

shaderc_spvc_compilation_result_t generate_vulkan_shader(
    const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options,
    shaderc_spvc_compilation_result_t result) {
  spirv_cross::CompilerReflection* compiler;

// spvc IR generation is under development, for now run spirv-cross
// compiler(SHADERC_ENABLE_SPVC_PARSER is OFF by default)
// TODO (sarahM0): change the default to spvc IR generation when it's done
#if SHADERC_ENABLE_SPVC_PARSER
  spirv_cross::ParsedIR ir;
  result = generate_spvcir(&ir, source, source_len, options, result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append(
        "Transformations between source and target "
        "execution environments failed (spvc-ir-pass).\n");
    return result;
  } else {
    compiler = new (std::nothrow) spirv_cross::CompilerReflection(ir);
  }
#else
  compiler =
      new (std::nothrow) spirv_cross::CompilerReflection(source, source_len);
#endif

  if (!compiler) {
    result->messages.append(
        "Unable to initialize SPIRV-Cross reflection "
        "compiler.\n");
    result->status = shaderc_compilation_status_compilation_error;
    return result;
  }
  result->compiler.reset(compiler);

  return result;
}

shaderc_spvc_compilation_result_t generate_spvcir(
    spirv_cross::ParsedIR* ir, const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options,
    shaderc_spvc_compilation_result_t result) {
#if SHADERC_ENABLE_SPVC_PARSER
  std::vector<uint32_t> binary_output;
  spvtools::Optimizer opt(options->source_env);
  opt.SetMessageConsumer(std::bind(
      consume_spirv_tools_message, result, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  ir->spirv = std::vector<uint32_t>(source, source + source_len);

  opt.RegisterPass(
      spvtools::Optimizer::PassToken(std::unique_ptr<spvtools::opt::Pass>(
          reinterpret_cast<spvtools::opt::Pass*>(
              new spvtools::opt::SpvcIrPass(ir)))));

  if (!opt.Run(source, source_len, &binary_output)) {
    result->status = shaderc_compilation_status_transformation_error;
  }
#endif  // SHADERC_ENABLE_SPVC_PARSER
  return result;
}

}  // namespace spvc_private
