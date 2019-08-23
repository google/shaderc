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
#include "spirv-cross/spirv_glsl.hpp"
#include "spirv-cross/spirv_hlsl.hpp"
#include "spirv-cross/spirv_msl.hpp"
#include "spirv-tools/libspirv.hpp"
#include "spirv-tools/optimizer.hpp"

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
  bool validate = true;
  bool remove_unused_variables = false;
  bool robust_buffer_access_pass = false;
  bool flatten_ubo = false;
  bool force_es = false;
  bool forced_es_setting = false;
  std::string entry_point;
  spv_target_env source_env = SPV_ENV_VULKAN_1_0;
  spv_target_env target_env = SPV_ENV_VULKAN_1_0;
  std::vector<uint32_t> msl_discrete_descriptor_sets;
  spirv_cross::CompilerGLSL::Options glsl;
  spirv_cross::CompilerHLSL::Options hlsl;
  spirv_cross::CompilerMSL::Options msl;
};

namespace {

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

// Test whether or not the given SPIR-V binary is valid for the specific
// environment. Invoke spirv-val to perform this operation.
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

// Convert SPIR-V from one environment to another, if there is a known
// conversion. If the origin and destination environments are the same, then
// the binary is just copied to the output buffer. Invokes spirv-opt to perform
// the actual translation.
shaderc_spvc_compilation_result_t translate_spirv(
    spv_target_env source_env, spv_target_env target_env,
    const uint32_t* source, size_t source_len, std::vector<uint32_t>* target,
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

  spvtools::Optimizer opt(target_env);
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

  if (!opt.Run(source, source_len, target)) {
    result->messages.append(
        "Transformations between source and target "
        "execution environments failed.\n");
    result->status = shaderc_compilation_status_transformation_error;
    return result;
  }

  return result;
}

// Execute the validation and translate steps. Specifically validates the input,
// transforms it, then validates the transformed input. All of these steps are
// only done if needed.
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
                           source_len, target, result);
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

// Given a configured compiler run it to generate a shader. Does all of the
// required trapping to handle if the compile fails.
shaderc_spvc_compilation_result_t generate_shader(
    spirv_cross::Compiler* compiler, shaderc_spvc_compilation_result_t result) {
  TRY_IF_EXCEPTIONS_ENABLED {
    result->output = compiler->compile();
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

// Given a Vulkan SPIR-V shader and set of options generate a GLSL shader.
// Handles correctly setting up the SPIRV-Cross compiler based on the options
// and then envoking it.
shaderc_spvc_compilation_result_t generate_glsl_shader(
    const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options,
    shaderc_spvc_compilation_result_t result) {
  std::unique_ptr<spirv_cross::CompilerGLSL> compiler(
      new (std::nothrow) spirv_cross::CompilerGLSL(source, source_len));
  if (!compiler) {
    result->messages.append(
        "Unable to initialize SPIRV-Cross GLSL compiler.\n");
    result->status = shaderc_compilation_status_compilation_error;
    return result;
  }

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

  result = generate_shader(compiler.get(), result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append("Compilation failed.  Partial source:\n");
    result->messages.append(compiler->get_partial_source());
  }

  return result;
}

// Given a Vulkan SPIR-V shader and set of options generate a HLSL shader.
// Handles correctly setting up the SPIRV-Cross compiler based on the options
// and then envoking it.
shaderc_spvc_compilation_result_t generate_hlsl_shader(
    const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options,
    shaderc_spvc_compilation_result_t result) {
  std::unique_ptr<spirv_cross::CompilerHLSL> compiler(
      new (std::nothrow) spirv_cross::CompilerHLSL(source, source_len));
  if (!compiler) {
    result->messages.append(
        "Unable to initialize SPIRV-Cross HLSL compiler.\n");
    result->status = shaderc_compilation_status_compilation_error;
    return result;
  }

  compiler->set_common_options(options->glsl);
  compiler->set_hlsl_options(options->hlsl);

  result = generate_shader(compiler.get(), result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append("Compilation failed.  Partial source:\n");
    result->messages.append(compiler->get_partial_source());
  }

  return result;
}

// Given a Vulkan SPIR-V shader and set of options generate a MSL shader.
// Handles correctly setting up the SPIRV-Cross compiler based on the options
// and then envoking it.
shaderc_spvc_compilation_result_t generate_msl_shader(
    const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options,
    shaderc_spvc_compilation_result_t result) {
  std::unique_ptr<spirv_cross::CompilerMSL> compiler(
      new (std::nothrow) spirv_cross::CompilerMSL(source, source_len));
  if (!compiler) {
    result->messages.append("Unable to initialize SPIRV-Cross MSL compiler.\n");
    result->status = shaderc_compilation_status_compilation_error;
    return result;
  }

  compiler->set_common_options(options->glsl);
  compiler->set_msl_options(options->msl);
  for (auto i : options->msl_discrete_descriptor_sets)
    compiler->add_discrete_descriptor_set(i);

  result = generate_shader(compiler.get(), result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append("Compilation failed.  Partial source:\n");
    result->messages.append(compiler->get_partial_source());
  }

  return result;
}

}  // namespace

shaderc_spvc_compile_options_t shaderc_spvc_compile_options_initialize() {
  shaderc_spvc_compile_options_t options =
      new (std::nothrow) shaderc_spvc_compile_options;
  if (options) {
    options->glsl.version = 0;
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

void shaderc_spvc_compile_options_set_source_env(
    shaderc_spvc_compile_options_t options, shaderc_target_env env,
    shaderc_env_version version) {
  options->source_env = get_spv_target_env(env, version);
}

void shaderc_spvc_compile_options_set_target_env(
    shaderc_spvc_compile_options_t options, shaderc_target_env env,
    shaderc_env_version version) {
  options->target_env = get_spv_target_env(env, version);
}

void shaderc_spvc_compile_options_set_entry_point(
    shaderc_spvc_compile_options_t options, const char* entry_point) {
  options->entry_point = entry_point;
}

void shaderc_spvc_compile_options_set_remove_unused_variables(
    shaderc_spvc_compile_options_t options, bool b) {
  options->remove_unused_variables = b;
}

void shaderc_spvc_compile_options_set_robust_buffer_access_pass(
    shaderc_spvc_compile_options_t options, bool b) {
  options->robust_buffer_access_pass = b;
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

void shaderc_spvc_compile_options_set_flatten_multidimensional_arrays(
    shaderc_spvc_compile_options_t options, bool b) {
  options->glsl.flatten_multidimensional_arrays = b;
}

void shaderc_spvc_compile_options_set_es(shaderc_spvc_compile_options_t options,
                                         bool b) {
  options->forced_es_setting = b;
  options->force_es = true;
}

void shaderc_spvc_compile_options_set_glsl_emit_push_constant_as_ubo(
    shaderc_spvc_compile_options_t options, bool b) {
  options->glsl.emit_push_constant_as_uniform_buffer = b;
}

void shaderc_spvc_compile_options_set_msl_language_version(
    shaderc_spvc_compile_options_t options, uint32_t version) {
  options->msl.msl_version = version;
}

void shaderc_spvc_compile_options_set_msl_swizzle_texture_samples(
    shaderc_spvc_compile_options_t options, bool b) {
  options->msl.swizzle_texture_samples = b;
}

void shaderc_spvc_compile_options_set_msl_platform(
    shaderc_spvc_compile_options_t options,
    shaderc_spvc_msl_platform platform) {
  switch (platform) {
    case shaderc_spvc_msl_platform_ios:
      options->msl.platform = spirv_cross::CompilerMSL::Options::iOS;
    break;
    case shaderc_spvc_msl_platform_macos:
      options->msl.platform = spirv_cross::CompilerMSL::Options::macOS;
    break;
  }
}

void shaderc_spvc_compile_options_set_msl_pad_fragment_output(
    shaderc_spvc_compile_options_t options, bool b) {
  options->msl.pad_fragment_output_components = b;
}

void shaderc_spvc_compile_options_set_msl_capture(
    shaderc_spvc_compile_options_t options, bool b) {
  options->msl.capture_output_to_buffer = b;
}

void shaderc_spvc_compile_options_set_msl_domain_lower_left(
    shaderc_spvc_compile_options_t options, bool b) {
  options->msl.tess_domain_origin_lower_left = b;
}

void shaderc_spvc_compile_options_set_msl_argument_buffers(
    shaderc_spvc_compile_options_t options, bool b) {
  options->msl.argument_buffers = b;
}

void shaderc_spvc_compile_options_set_msl_discrete_descriptor_sets(
    shaderc_spvc_compile_options_t options, const uint32_t* descriptors,
    size_t num_descriptors) {
  options->msl_discrete_descriptor_sets.resize(num_descriptors);
  std::copy_n(descriptors, num_descriptors,
              options->msl_discrete_descriptor_sets.begin());
}

void shaderc_spvc_compile_options_set_hlsl_shader_model(
    shaderc_spvc_compile_options_t options, uint32_t model) {
  options->hlsl.shader_model = model;
}

void shaderc_spvc_compile_options_set_hlsl_point_size_compat(
    shaderc_spvc_compile_options_t options, bool b) {
  options->hlsl.point_size_compat = b;
}

void shaderc_spvc_compile_options_set_hlsl_point_coord_compat(
    shaderc_spvc_compile_options_t options, bool b) {
  options->hlsl.point_coord_compat = b;
}

void shaderc_spvc_compile_options_set_fixup_clipspace(
    shaderc_spvc_compile_options_t options, bool b) {
  options->glsl.vertex.fixup_clipspace = b;
}

void shaderc_spvc_compile_options_set_flip_vert_y(
    shaderc_spvc_compile_options_t options, bool b) {
  options->glsl.vertex.flip_vert_y = b;
}

void shaderc_spvc_compile_options_set_validate(
    shaderc_spvc_compile_options_t options, bool b) {
  options->validate = b;
}

size_t shaderc_spvc_compile_options_set_for_fuzzing(
    shaderc_spvc_compile_options_t options, const uint8_t* data, size_t size) {
  if (!data || size < sizeof(*options)) return 0;

  memcpy(static_cast<void*>(options), data, sizeof(*options));
  return sizeof(*options);
}

shaderc_spvc_compiler_t shaderc_spvc_compiler_initialize() {
  return new (std::nothrow) shaderc_spvc_compiler;
}

void shaderc_spvc_compiler_release(shaderc_spvc_compiler_t compiler) {
  delete compiler;
}

shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_glsl(
    const shaderc_spvc_compiler_t, const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options) {
  auto* result = new (std::nothrow) shaderc_spvc_compilation_result;
  if (!result) return nullptr;
  result->status = shaderc_compilation_status_success;

  std::vector<uint32_t> intermediate_source;
  result = validate_and_translate_spirv(source, source_len, options,
                                        &intermediate_source, result);
  if (result->status != shaderc_compilation_status_success) {
    return result;
  }

  result = generate_glsl_shader(intermediate_source.data(),
                                intermediate_source.size(), options, result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append(
        "Generation of GLSL from transformed source failed.\n");
  }

  return result;
}

shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_hlsl(
    const shaderc_spvc_compiler_t, const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options) {
  auto* result = new (std::nothrow) shaderc_spvc_compilation_result;
  if (!result) return nullptr;
  result->status = shaderc_compilation_status_success;

  std::vector<uint32_t> intermediate_source;
  result = validate_and_translate_spirv(source, source_len, options,
                                        &intermediate_source, result);
  if (result->status != shaderc_compilation_status_success) {
    return result;
  }

  result = generate_hlsl_shader(intermediate_source.data(),
                                intermediate_source.size(), options, result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append(
        "Generation of HLSL from transformed source failed.\n");
  }

  return result;
}

shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_msl(
    const shaderc_spvc_compiler_t, const uint32_t* source, size_t source_len,
    shaderc_spvc_compile_options_t options) {
  auto* result = new (std::nothrow) shaderc_spvc_compilation_result;
  if (!result) return nullptr;
  result->status = shaderc_compilation_status_success;

  std::vector<uint32_t> intermediate_source;
  result = validate_and_translate_spirv(source, source_len, options,
                                        &intermediate_source, result);
  if (result->status != shaderc_compilation_status_success) {
    return result;
  }

  result = generate_msl_shader(intermediate_source.data(),
                               intermediate_source.size(), options, result);
  if (result->status != shaderc_compilation_status_success) {
    result->messages.append(
        "Generation of MSL from transformed source failed.\n");
  }

  return result;
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
