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

#include <spvc/spvc.hpp>

#include "spvc_private.h"

namespace {

spv::ExecutionModel spvc_model_to_spv_model(
    shaderc_spvc_execution_model model) {
  switch (model) {
    case shaderc_spvc_execution_model_vertex:
      return spv::ExecutionModel::ExecutionModelVertex;
    case shaderc_spvc_execution_model_fragment:
      return spv::ExecutionModel::ExecutionModelFragment;
    case shaderc_spvc_execution_model_glcompute:
      return spv::ExecutionModel::ExecutionModelGLCompute;
    case shaderc_spvc_execution_model_invalid:
      return spv::ExecutionModel::ExecutionModelMax;
  }

  // Older gcc doesn't recognize that all of the possible cases are covered
  // above.
  assert(false);
  return spv::ExecutionModel::ExecutionModelMax;
}

shaderc_spvc_execution_model spv_model_to_spvc_model(
    spv::ExecutionModel model) {
  switch (model) {
    case spv::ExecutionModel::ExecutionModelVertex:
      return shaderc_spvc_execution_model_vertex;
    case spv::ExecutionModel::ExecutionModelFragment:
      return shaderc_spvc_execution_model_fragment;
    case spv::ExecutionModel::ExecutionModelGLCompute:
      return shaderc_spvc_execution_model_glcompute;
    default:
      return shaderc_spvc_execution_model_invalid;
  }
}

const spirv_cross::SmallVector<spirv_cross::Resource>* get_shader_resources(
    const spirv_cross::ShaderResources& resources,
    shaderc_spvc_shader_resource resource) {
  switch (resource) {
    case shaderc_spvc_shader_resource_uniform_buffers:
      return &(resources.uniform_buffers);
    case shaderc_spvc_shader_resource_separate_images:
      return &(resources.separate_images);
    case shaderc_spvc_shader_resource_separate_samplers:
      return &(resources.separate_samplers);
    case shaderc_spvc_shader_resource_storage_buffers:
      return &(resources.storage_buffers);
  }

  // Older gcc doesn't recognize that all of the possible cases are covered
  // above.
  assert(false);
  return nullptr;
}

shaderc_spvc_texture_view_dimension spirv_dim_to_texture_view_dimension(
    spv::Dim dim, bool arrayed) {
  switch (dim) {
    case spv::Dim::Dim1D:
      return shaderc_spvc_texture_view_dimension_e1D;
    case spv::Dim::Dim2D:
      if (arrayed) {
        return shaderc_spvc_texture_view_dimension_e2D_array;
      } else {
        return shaderc_spvc_texture_view_dimension_e2D;
      }
    case spv::Dim::Dim3D:
      return shaderc_spvc_texture_view_dimension_e3D;
    case spv::Dim::DimCube:
      if (arrayed) {
        return shaderc_spvc_texture_view_dimension_cube_array;
      } else {
        return shaderc_spvc_texture_view_dimension_cube;
      }
    default:
      return shaderc_spvc_texture_view_dimension_undefined;
  }
}

shaderc_spvc_texture_format_type spirv_cross_base_type_to_texture_format_type(
    spirv_cross::SPIRType::BaseType type) {
  switch (type) {
    case spirv_cross::SPIRType::Float:
      return shaderc_spvc_texture_format_type_float;
    case spirv_cross::SPIRType::Int:
      return shaderc_spvc_texture_format_type_sint;
    case spirv_cross::SPIRType::UInt:
      return shaderc_spvc_texture_format_type_uint;
    default:
      return shaderc_spvc_texture_format_type_other;
  }
}

shaderc_spvc_status get_location_info_impl(
    spirv_cross::Compiler* compiler,
    const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
    shaderc_spvc_resource_location_info* locations, size_t* location_count) {
  *location_count = resources.size();
  if (!locations) return shaderc_spvc_status_success;

  for (const auto& resource : resources) {
    if (!compiler->get_decoration_bitset(resource.id)
             .get(spv::DecorationLocation)) {
      return shaderc_spvc_status_internal_error;
    }
    locations->id = resource.id;
    if (compiler->get_decoration_bitset(resource.id)
            .get(spv::DecorationLocation)) {
      locations->location =
          compiler->get_decoration(resource.id, spv::DecorationLocation);
      locations->has_location = true;
    } else {
      locations->has_location = false;
    }
    locations++;
  }
  return shaderc_spvc_status_success;
}

}  // namespace

shaderc_spvc_context_t shaderc_spvc_context_create() {
  return new (std::nothrow) shaderc_spvc_context;
}

void shaderc_spvc_context_destroy(shaderc_spvc_context_t context) {
  if (context) delete context;
}

const char* shaderc_spvc_context_get_messages(
    const shaderc_spvc_context_t context) {
  for (const auto& message : context->messages) {
    context->messages_string += message;
    context->messages_string += "\n";
  }
  context->messages.clear();
  return context->messages_string.c_str();
}

void* shaderc_spvc_context_get_compiler(const shaderc_spvc_context_t context) {
  return context->cross_compiler.get();
}

void shaderc_spvc_context_set_use_spvc_parser(shaderc_spvc_context_t context,
                                              bool b) {
  context->use_spvc_parser = b;
}

shaderc_spvc_compile_options_t shaderc_spvc_compile_options_create() {
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
  return shaderc_spvc_compile_options_create();
}

void shaderc_spvc_compile_options_destroy(
    shaderc_spvc_compile_options_t options) {
  if (options) delete options;
}

void shaderc_spvc_compile_options_set_source_env(
    shaderc_spvc_compile_options_t options, shaderc_target_env env,
    shaderc_env_version version) {
  options->source_env = spvc_private::get_spv_target_env(env, version);
}

void shaderc_spvc_compile_options_set_target_env(
    shaderc_spvc_compile_options_t options, shaderc_target_env env,
    shaderc_env_version version) {
  options->target_env = spvc_private::get_spv_target_env(env, version);
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

void shaderc_spvc_compile_options_set_emit_line_directives(
    shaderc_spvc_compile_options_t options, bool b) {
  options->glsl.emit_line_directives = b;
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

void shaderc_spvc_compile_options_set_msl_enable_point_size_builtin(
    shaderc_spvc_compile_options_t options, bool b) {
  options->msl.enable_point_size_builtin = b;
}

void shaderc_spvc_compile_options_set_msl_buffer_size_buffer_index(
    shaderc_spvc_compile_options_t options, uint32_t index) {
  options->msl.buffer_size_buffer_index = index;
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

void shaderc_spvc_compile_options_set_optimize(
    shaderc_spvc_compile_options_t options, bool b) {
  options->optimize = b;
}

size_t shaderc_spvc_compile_options_set_for_fuzzing(
    shaderc_spvc_compile_options_t options, const uint8_t* data, size_t size) {
  if (!data || size < sizeof(*options)) return 0;

  memcpy(static_cast<void*>(options), data, sizeof(*options));
  return sizeof(*options);
}

shaderc_spvc_status shaderc_spvc_initialize_impl(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options,
    shaderc_spvc_status (*generator)(const shaderc_spvc_context_t,
                                     const uint32_t*, size_t,
                                     shaderc_spvc_compile_options_t)) {
  if (!context) return shaderc_spvc_status_configuration_error;
  shaderc_spvc_status status = spvc_private::validate_and_translate_spirv(
      context, source, source_len, options, &context->intermediate_shader);
  if (status != shaderc_spvc_status_success) return status;

  status = generator(context, context->intermediate_shader.data(),
                     context->intermediate_shader.size(), options);
  if (status != shaderc_spvc_status_success) return status;

  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_initialize_for_glsl(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {
  context->target_lang = SPVC_TARGET_LANG_GLSL;
  return shaderc_spvc_initialize_impl(context, source, source_len, options,
                                      spvc_private::generate_glsl_compiler);
}

shaderc_spvc_status shaderc_spvc_initialize_for_hlsl(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {
  context->target_lang = SPVC_TARGET_LANG_HLSL;
  return shaderc_spvc_initialize_impl(context, source, source_len, options,
                                      spvc_private::generate_hlsl_compiler);
}

shaderc_spvc_status shaderc_spvc_initialize_for_msl(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {
  context->target_lang = SPVC_TARGET_LANG_MSL;
  return shaderc_spvc_initialize_impl(context, source, source_len, options,
                                      spvc_private::generate_msl_compiler);
}

shaderc_spvc_status shaderc_spvc_initialize_for_vulkan(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {
  context->target_lang = SPVC_TARGET_LANG_VULKAN;
  return shaderc_spvc_initialize_impl(context, source, source_len, options,
                                      spvc_private::generate_vulkan_compiler);
}

shaderc_spvc_status shaderc_spvc_compile_shader(
    const shaderc_spvc_context_t context,
    shaderc_spvc_compilation_result_t result) {
  if (!context->cross_compiler ||
      context->target_lang == SPVC_TARGET_LANG_UNKNOWN) {
    return shaderc_spvc_status_configuration_error;
  }

  if (context->target_lang == SPVC_TARGET_LANG_VULKAN) {
    // No actual cross compilation is needed, since the intermediate shader is
    // already in Vulkan SPIR->V.
    result->binary_output = context->intermediate_shader;
    return shaderc_spvc_status_success;
  } else {
    shaderc_spvc_status status =
        spvc_private::generate_shader(context->cross_compiler.get(), result);
    if (status != shaderc_spvc_status_success) {
      context->messages.push_back("Compilation failed.  Partial source:");
      if (context->target_lang == SPVC_TARGET_LANG_GLSL) {
        spirv_cross::CompilerGLSL* cast_compiler =
            reinterpret_cast<spirv_cross::CompilerGLSL*>(
                context->cross_compiler.get());
        context->messages.push_back(cast_compiler->get_partial_source());
      } else if (context->target_lang == SPVC_TARGET_LANG_HLSL) {
        spirv_cross::CompilerHLSL* cast_compiler =
            reinterpret_cast<spirv_cross::CompilerHLSL*>(
                context->cross_compiler.get());
        context->messages.push_back(cast_compiler->get_partial_source());
      } else if (context->target_lang == SPVC_TARGET_LANG_MSL) {
        spirv_cross::CompilerMSL* cast_compiler =
            reinterpret_cast<spirv_cross::CompilerMSL*>(
                context->cross_compiler.get());
        context->messages.push_back(cast_compiler->get_partial_source());
      } else {
        context->messages.push_back("Unexpected target language in context");
      }
      context->cross_compiler.reset();
    }
    return status;
  }
}

shaderc_spvc_status shaderc_spvc_set_decoration(
    const shaderc_spvc_context_t context, uint32_t id,
    shaderc_spvc_decoration decoration, uint32_t argument) {
  spv::Decoration spirv_cross_decoration;
  shaderc_spvc_status status =
      spvc_private::shaderc_spvc_decoration_to_spirv_cross_decoration(
          decoration, &spirv_cross_decoration);
  if (status == shaderc_spvc_status_success) {
    context->cross_compiler->set_decoration(static_cast<spirv_cross::ID>(id),
                                            spirv_cross_decoration, argument);
  } else {
    context->messages.push_back(
        "Decoration Conversion failed.  shaderc_spvc_decoration not "
        "supported.");
  }
  return status;
}

shaderc_spvc_status shaderc_spvc_get_decoration(
    const shaderc_spvc_context_t context, uint32_t id,
    shaderc_spvc_decoration decoration, uint32_t* value) {
  if (!context) return shaderc_spvc_status_missing_context_error;

  if (!context->cross_compiler) {
    context->messages.push_back(
        "Invoked get_decoration without an initialized compiler");
    return shaderc_spvc_status_uninitialized_compiler_error;
  }

  if (!value) {
    context->messages.push_back(
        "Invoked get_decoration without a valid out param");
    return shaderc_spvc_status_invalid_out_param;
  }

  spv::Decoration spirv_cross_decoration;
  shaderc_spvc_status status =
      spvc_private::shaderc_spvc_decoration_to_spirv_cross_decoration(
          decoration, &spirv_cross_decoration);
  if (status != shaderc_spvc_status_success) {
    context->messages.push_back(
        "Decoration conversion failed.  shaderc_spvc_decoration not "
        "supported.");

    return status;
  }

  *value = context->cross_compiler->get_decoration(
      static_cast<spirv_cross::ID>(id), spirv_cross_decoration);
  if (*value == 0) {
    context->messages.push_back("Getting decoration failed. id not found.  ");
    return shaderc_spvc_status_compilation_error;
  }

  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_unset_decoration(
    const shaderc_spvc_context_t context, uint32_t id,
    shaderc_spvc_decoration decoration) {
  spv::Decoration spirv_cross_decoration;
  shaderc_spvc_status status =
      spvc_private::shaderc_spvc_decoration_to_spirv_cross_decoration(
          decoration, &spirv_cross_decoration);
  if (status == shaderc_spvc_status_success) {
    context->cross_compiler->unset_decoration(static_cast<spirv_cross::ID>(id),
                                              spirv_cross_decoration);
  } else {
    context->messages.push_back(
        "Decoration conversion failed.  shaderc_spvc_decoration not "
        "supported.");
  }
  return status;
}

void shaderc_spvc_get_combined_image_samplers(
    const shaderc_spvc_context_t context,
    shaderc_spvc_combined_image_sampler* samplers, size_t* num_samplers) {
  assert(num_samplers);
  if (!num_samplers) return;

  *num_samplers = context->cross_compiler->get_combined_image_samplers().size();
  if (!samplers) return;

  for (const auto& combined :
       context->cross_compiler->get_combined_image_samplers()) {
    samplers->combined_id = combined.combined_id;
    samplers->image_id = combined.image_id;
    samplers->sampler_id = combined.sampler_id;
    samplers++;
  }
}

void shaderc_spvc_set_name(const shaderc_spvc_context_t context, uint32_t id,
                           const char* name) {
  context->cross_compiler->set_name(id, name);
  return;
}

shaderc_spvc_status shaderc_spvc_add_msl_resource_binding(
    const shaderc_spvc_context_t context,
    const shaderc_spvc_msl_resource_binding binding) {
  if (!context) return shaderc_spvc_status_missing_context_error;

  if (!context->cross_compiler) {
    context->messages.push_back(
        "Invoked add_msl_resource_binding without an initialized compiler");
    return shaderc_spvc_status_uninitialized_compiler_error;
  }

  if (context->target_lang != SPVC_TARGET_LANG_MSL) {
    context->messages.push_back(
        "Invoked add_msl_resource_binding when target language was not MSL");
    return shaderc_spvc_status_configuration_error;
  }

  spirv_cross::MSLResourceBinding cross_binding;
  cross_binding.stage = spvc_model_to_spv_model(binding.stage);
  cross_binding.binding = binding.binding;
  cross_binding.desc_set = binding.desc_set;
  cross_binding.msl_buffer = binding.msl_buffer;
  cross_binding.msl_texture = binding.msl_texture;
  cross_binding.msl_sampler = binding.msl_sampler;
  reinterpret_cast<spirv_cross::CompilerMSL*>(context->cross_compiler.get())
      ->add_msl_resource_binding(cross_binding);

  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_get_workgroup_size(
    const shaderc_spvc_context_t context, const char* function_name,
    shaderc_spvc_execution_model execution_model,
    shaderc_spvc_workgroup_size* workgroup_size) {
  if (!context) return shaderc_spvc_status_missing_context_error;

  if (!context->cross_compiler) {
    context->messages.push_back(
        "Invoked get_workgroup_size without an initialized compiler");
    return shaderc_spvc_status_uninitialized_compiler_error;
  }

  if (!workgroup_size) {
    context->messages.push_back(
        "Invoked get_workgroup_size without a valid out param");
    return shaderc_spvc_status_invalid_out_param;
  }

  const auto& cross_size =
      context->cross_compiler
          ->get_entry_point(function_name,
                            spvc_model_to_spv_model(execution_model))
          .workgroup_size;
  workgroup_size->x = cross_size.x;
  workgroup_size->y = cross_size.y;
  workgroup_size->z = cross_size.z;
  workgroup_size->constant = cross_size.constant;

  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_needs_buffer_size_buffer(
    const shaderc_spvc_context_t context, bool* b) {
  if (!context) return shaderc_spvc_status_missing_context_error;

  if (!context->cross_compiler) {
    context->messages.push_back(
        "Invoked needs_buffer_size_buffer without an initialized compiler");
    return shaderc_spvc_status_uninitialized_compiler_error;
  }

  if (context->target_lang != SPVC_TARGET_LANG_MSL) {
    context->messages.push_back(
        "Invoked needs_buffer_size_buffer when target language was not MSL");
    return shaderc_spvc_status_configuration_error;
  }

  if (!b) {
    context->messages.push_back(
        "Invoked needs_buffer_size_buffer without a valid out param");
    return shaderc_spvc_status_invalid_out_param;
  }

  *b =
      reinterpret_cast<spirv_cross::CompilerMSL*>(context->cross_compiler.get())
          ->needs_output_buffer();

  return shaderc_spvc_status_success;
}

void shaderc_spvc_build_combined_image_samplers(
    const shaderc_spvc_context_t context) {
  context->cross_compiler->build_combined_image_samplers();
}

shaderc_spvc_status shaderc_spvc_get_execution_model(
    const shaderc_spvc_context_t context,
    shaderc_spvc_execution_model* execution_model) {
  if (!context) return shaderc_spvc_status_missing_context_error;

  if (!context->cross_compiler) {
    context->messages.push_back(
        "Invoked get_execution_model without an initialized compiler");
    return shaderc_spvc_status_uninitialized_compiler_error;
  }

  if (!execution_model) {
    context->messages.push_back(
        "Invoked get_execution_model without a valid out param");
    return shaderc_spvc_status_invalid_out_param;
  }

  auto spirv_model = context->cross_compiler->get_execution_model();
  *execution_model = spv_model_to_spvc_model(spirv_model);
  if (*execution_model == shaderc_spvc_execution_model_invalid) {
    context->messages.push_back(
        "Shader execution model appears to be of an unsupported type");
    return shaderc_spvc_status_internal_error;
  }

  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_get_push_constant_buffer_count(
    const shaderc_spvc_context_t context, size_t* count) {
  if (!context) return shaderc_spvc_status_missing_context_error;

  if (!context->cross_compiler) {
    context->messages.push_back(
        "Invoked push_constant_buffer_count without an initialized compiler");
    return shaderc_spvc_status_uninitialized_compiler_error;
  }

  if (!count) {
    context->messages.push_back(
        "Invoked push_constant_buffer_count without a valid out param");
    return shaderc_spvc_status_invalid_out_param;
  }

  *count = context->cross_compiler->get_shader_resources()
               .push_constant_buffers.size();
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_get_binding_info(
    const shaderc_spvc_context_t context, shaderc_spvc_shader_resource resource,
    shaderc_spvc_binding_type binding_type, shaderc_spvc_binding_info* bindings,
    size_t* binding_count) {
  if (!context) return shaderc_spvc_status_missing_context_error;

  if (!context->cross_compiler) {
    context->messages.push_back(
        "Invoked get_binding_info without an initialized compiler");
    return shaderc_spvc_status_uninitialized_compiler_error;
  }
  auto* compiler = context->cross_compiler.get();

  if (!binding_count) {
    context->messages.push_back(
        "Invoked get_binding_info without a valid binding_count "
        "param");
    return shaderc_spvc_status_invalid_out_param;
  }

  const auto& resources = compiler->get_shader_resources();
  const auto* shader_resources = get_shader_resources(resources, resource);
  *binding_count = shader_resources->size();
  if (!bindings) return shaderc_spvc_status_success;

  for (const auto& shader_resource : *shader_resources) {
    bindings->texture_dimension = shaderc_spvc_texture_view_dimension_undefined;
    bindings->texture_component_type = shaderc_spvc_texture_format_type_float;

    if (!compiler->get_decoration_bitset(shader_resource.id)
             .get(spv::DecorationBinding)) {
      context->messages.push_back(
          "Unable to get binding decoration for shader resource");
      return shaderc_spvc_status_internal_error;
    }
    uint32_t binding_decoration =
        compiler->get_decoration(shader_resource.id, spv::DecorationBinding);
    bindings->binding = binding_decoration;

    if (!compiler->get_decoration_bitset(shader_resource.id)
             .get(spv::DecorationDescriptorSet)) {
      context->messages.push_back(
          "Unable to get descriptor set decoration for shader resource");
      return shaderc_spvc_status_internal_error;
    }
    uint32_t descriptor_set_decoration = compiler->get_decoration(
        shader_resource.id, spv::DecorationDescriptorSet);
    bindings->set = descriptor_set_decoration;

    bindings->id = shader_resource.id;
    bindings->base_type_id = shader_resource.base_type_id;

    switch (binding_type) {
      case shaderc_spvc_binding_type_sampled_texture: {
        spirv_cross::SPIRType::ImageType imageType =
            compiler->get_type(bindings->base_type_id).image;
        spirv_cross::SPIRType::BaseType textureComponentType =
            compiler->get_type(imageType.type).basetype;
        bindings->multisampled = imageType.ms;
        bindings->texture_dimension = spirv_dim_to_texture_view_dimension(
            imageType.dim, imageType.arrayed);
        bindings->texture_component_type =
            spirv_cross_base_type_to_texture_format_type(textureComponentType);
        bindings->binding_type = binding_type;
      } break;
      case shaderc_spvc_binding_type_storage_buffer: {
        // Differentiate between readonly storage bindings and writable ones
        // based on the NonWritable decoration
        spirv_cross::Bitset flags =
            compiler->get_buffer_block_flags(shader_resource.id);
        if (flags.get(spv::DecorationNonWritable)) {
          bindings->binding_type =
              shaderc_spvc_binding_type_readonly_storage_buffer;
        } else {
          bindings->binding_type = shaderc_spvc_binding_type_storage_buffer;
        }
      } break;
      default:
        bindings->binding_type = binding_type;
    }
    bindings++;
  }

  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_get_input_stage_location_info(
    const shaderc_spvc_context_t context,
    shaderc_spvc_resource_location_info* locations, size_t* location_count) {
  if (!context) return shaderc_spvc_status_missing_context_error;

  if (!context->cross_compiler) {
    context->messages.push_back(
        "Invoked get_input_stage_location_info without an initialized "
        "compiler");
    return shaderc_spvc_status_uninitialized_compiler_error;
  }
  auto* compiler = context->cross_compiler.get();

  if (!location_count) {
    context->messages.push_back(
        "Invoked get_input_stage_location_info without a valid location_count "
        "param");
    return shaderc_spvc_status_invalid_out_param;
  }

  shaderc_spvc_status status = get_location_info_impl(
      compiler, compiler->get_shader_resources().stage_inputs, locations,
      location_count);
  if (status != shaderc_spvc_status_success) {
    context->messages.push_back(
        "Unable to get location decoration for stage input");
  }

  return status;
}

shaderc_spvc_status shaderc_spvc_get_output_stage_location_info(
    const shaderc_spvc_context_t context,
    shaderc_spvc_resource_location_info* locations, size_t* location_count) {
  if (!context) return shaderc_spvc_status_missing_context_error;

  if (!context->cross_compiler) {
    context->messages.push_back(
        "Invoked get_output_stage_location_info without an initialized "
        "compiler");
    return shaderc_spvc_status_uninitialized_compiler_error;
  }
  auto* compiler = context->cross_compiler.get();

  if (!location_count) {
    context->messages.push_back(
        "Invoked get_output_stage_location_info without a valid location_count "
        "param");
    return shaderc_spvc_status_invalid_out_param;
  }

  shaderc_spvc_status status = get_location_info_impl(
      compiler, compiler->get_shader_resources().stage_outputs, locations,
      location_count);
  if (status != shaderc_spvc_status_success) {
    context->messages.push_back(
        "Unable to get location decoration for stage output");
  }

  return status;
}

shaderc_spvc_status shaderc_spvc_get_output_stage_type_info(
    const shaderc_spvc_context_t context,
    shaderc_spvc_resource_type_info* types, size_t* type_count) {
  if (!context) return shaderc_spvc_status_missing_context_error;

  if (!context->cross_compiler) {
    context->messages.push_back(
        "Invoked get_output_stage_type_info without an initialized compiler");
    return shaderc_spvc_status_uninitialized_compiler_error;
  }
  auto* compiler = context->cross_compiler.get();

  if (!type_count) {
    context->messages.push_back(
        "Invoked get_output_stage_type_info without a valid location_count "
        "param");
    return shaderc_spvc_status_invalid_out_param;
  }
  const auto& resources = compiler->get_shader_resources().stage_outputs;

  *type_count = resources.size();
  if (!types) return shaderc_spvc_status_success;

  for (const auto& resource : resources) {
    if (!compiler->get_decoration_bitset(resource.id)
             .get(spv::DecorationLocation)) {
      context->messages.push_back(
          "Unable to get location decoration for stage output");
      return shaderc_spvc_status_internal_error;
    }

    types->location =
        compiler->get_decoration(resource.id, spv::DecorationLocation);
    spirv_cross::SPIRType::BaseType base_type =
        compiler->get_type(resource.base_type_id).basetype;
    types->type = spirv_cross_base_type_to_texture_format_type(base_type);
    types++;
  }

  return shaderc_spvc_status_success;
}

shaderc_spvc_compilation_result_t shaderc_spvc_result_create() {
  return new (std::nothrow) shaderc_spvc_compilation_result;
}

void shaderc_spvc_result_destroy(shaderc_spvc_compilation_result_t result) {
  if (result) delete result;
}

const char* shaderc_spvc_result_get_string_output(
    const shaderc_spvc_compilation_result_t result) {
  return result->string_output.c_str();
}

const uint32_t* shaderc_spvc_result_get_binary_output(
    const shaderc_spvc_compilation_result_t result) {
  return result->binary_output.data();
}

uint32_t shaderc_spvc_result_get_binary_length(
    const shaderc_spvc_compilation_result_t result) {
  return result->binary_output.size();
}
