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

#include "spvc_log.h"
#include "spvc_private.h"

// MSVC 2013 doesn't define __func__
#ifndef __func__
#define __func__ __FUNCTION__
#endif

#define CHECK_CONTEXT(context)                                            \
  do {                                                                    \
    if (!context) {                                                       \
      shaderc_spvc::ErrorLog(nullptr)                                     \
          << "Invoked " << __func__ << " without an initialized context"; \
      return shaderc_spvc_status_missing_context_error;                   \
    }                                                                     \
  } while (0)

#define CHECK_CROSS_COMPILER(context, cross_compiler)          \
  do {                                                         \
    if (!cross_compiler) {                                     \
      shaderc_spvc::ErrorLog(context)                          \
          << "Invoked " << __func__                            \
          << " without an initialized cross compiler";         \
      return shaderc_spvc_status_uninitialized_compiler_error; \
    }                                                          \
  } while (0)

#define CHECK_OPTIONS(context, options)                                   \
  do {                                                                    \
    if (!options) {                                                       \
      shaderc_spvc::ErrorLog(context)                                     \
          << "Invoked " << __func__ << " without an initialized options"; \
      return shaderc_spvc_status_missing_options_error;                   \
    }                                                                     \
  } while (0)

#define CHECK_RESULT(context, result)                                    \
  do {                                                                   \
    if (!result) {                                                       \
      shaderc_spvc::ErrorLog(context)                                    \
          << "Invoked " << __func__ << " without an initialized result"; \
      return shaderc_spvc_status_missing_result_error;                   \
    }                                                                    \
  } while (0)

#define CHECK_OUT_PARAM(context, param, param_str)                 \
  do {                                                             \
    if (!param) {                                                  \
      shaderc_spvc::ErrorLog(context)                              \
          << "Invoked " << __func__ << " with invalid out param, " \
          << param_str;                                            \
      return shaderc_spvc_status_invalid_out_param;                \
    }                                                              \
  } while (0)

#define CHECK_IN_PARAM(context, param, param_str)                 \
  do {                                                            \
    if (!param) {                                                 \
      shaderc_spvc::ErrorLog(context)                             \
          << "Invoked " << __func__ << " with invalid in param, " \
          << param_str;                                           \
      return shaderc_spvc_status_invalid_in_param;                \
    }                                                             \
  } while (0)

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
    case shaderc_spvc_shader_resource_storage_images:
      return &(resources.storage_images);
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

shaderc_spvc_storage_texture_format spv_image_format_to_storage_texture_format(
    spv::ImageFormat format) {
  switch (format) {
    case spv::ImageFormatR8:
      return shaderc_spvc_storage_texture_format_r8unorm;
    case spv::ImageFormatR8Snorm:
      return shaderc_spvc_storage_texture_format_r8snorm;
    case spv::ImageFormatR8ui:
      return shaderc_spvc_storage_texture_format_r8uint;
    case spv::ImageFormatR8i:
      return shaderc_spvc_storage_texture_format_r8sint;
    case spv::ImageFormatR16ui:
      return shaderc_spvc_storage_texture_format_r16uint;
    case spv::ImageFormatR16i:
      return shaderc_spvc_storage_texture_format_r16sint;
    case spv::ImageFormatR16f:
      return shaderc_spvc_storage_texture_format_r16float;
    case spv::ImageFormatRg8:
      return shaderc_spvc_storage_texture_format_rg8unorm;
    case spv::ImageFormatRg8Snorm:
      return shaderc_spvc_storage_texture_format_rg8snorm;
    case spv::ImageFormatRg8ui:
      return shaderc_spvc_storage_texture_format_rg8uint;
    case spv::ImageFormatRg8i:
      return shaderc_spvc_storage_texture_format_rg8sint;
    case spv::ImageFormatR32f:
      return shaderc_spvc_storage_texture_format_r32float;
    case spv::ImageFormatR32ui:
      return shaderc_spvc_storage_texture_format_r32uint;
    case spv::ImageFormatR32i:
      return shaderc_spvc_storage_texture_format_r32sint;
    case spv::ImageFormatRg16ui:
      return shaderc_spvc_storage_texture_format_rg16uint;
    case spv::ImageFormatRg16i:
      return shaderc_spvc_storage_texture_format_rg16sint;
    case spv::ImageFormatRg16f:
      return shaderc_spvc_storage_texture_format_rg16float;
    case spv::ImageFormatRgba8:
      return shaderc_spvc_storage_texture_format_rgba8unorm;
    case spv::ImageFormatRgba8Snorm:
      return shaderc_spvc_storage_texture_format_rgba8snorm;
    case spv::ImageFormatRgba8ui:
      return shaderc_spvc_storage_texture_format_rgba8uint;
    case spv::ImageFormatRgba8i:
      return shaderc_spvc_storage_texture_format_rgba8sint;
    case spv::ImageFormatRgb10A2:
      return shaderc_spvc_storage_texture_format_rgb10a2unorm;
    case spv::ImageFormatR11fG11fB10f:
      return shaderc_spvc_storage_texture_format_rg11b10float;
    case spv::ImageFormatRg32f:
      return shaderc_spvc_storage_texture_format_rg32float;
    case spv::ImageFormatRg32ui:
      return shaderc_spvc_storage_texture_format_rg32uint;
    case spv::ImageFormatRg32i:
      return shaderc_spvc_storage_texture_format_rg32sint;
    case spv::ImageFormatRgba16ui:
      return shaderc_spvc_storage_texture_format_rgba16uint;
    case spv::ImageFormatRgba16i:
      return shaderc_spvc_storage_texture_format_rgba16sint;
    case spv::ImageFormatRgba16f:
      return shaderc_spvc_storage_texture_format_rgba16float;
    case spv::ImageFormatRgba32f:
      return shaderc_spvc_storage_texture_format_rgba32float;
    case spv::ImageFormatRgba32ui:
      return shaderc_spvc_storage_texture_format_rgba32uint;
    case spv::ImageFormatRgba32i:
      return shaderc_spvc_storage_texture_format_rgba32sint;
    default:
      return shaderc_spvc_storage_texture_format_undefined;
  }
}

spv_target_env shaderc_spvc_spv_env_to_spv_target_env(
    shaderc_spvc_spv_env env) {
  switch (env) {
    case shaderc_spvc_spv_env_universal_1_0:
      return SPV_ENV_UNIVERSAL_1_0;
    case shaderc_spvc_spv_env_vulkan_1_0:
      return SPV_ENV_VULKAN_1_0;
    case shaderc_spvc_spv_env_universal_1_1:
      return SPV_ENV_UNIVERSAL_1_1;
    case shaderc_spvc_spv_env_opencl_2_1:
      return SPV_ENV_OPENCL_2_1;
    case shaderc_spvc_spv_env_opencl_2_2:
      return SPV_ENV_OPENCL_2_2;
    case shaderc_spvc_spv_env_opengl_4_0:
      return SPV_ENV_OPENGL_4_0;
    case shaderc_spvc_spv_env_opengl_4_1:
      return SPV_ENV_OPENGL_4_1;
    case shaderc_spvc_spv_env_opengl_4_2:
      return SPV_ENV_OPENGL_4_2;
    case shaderc_spvc_spv_env_opengl_4_3:
      return SPV_ENV_OPENGL_4_3;
    case shaderc_spvc_spv_env_opengl_4_5:
      return SPV_ENV_OPENGL_4_5;
    case shaderc_spvc_spv_env_universal_1_2:
      return SPV_ENV_UNIVERSAL_1_2;
    case shaderc_spvc_spv_env_opencl_1_2:
      return SPV_ENV_OPENCL_1_2;
    case shaderc_spvc_spv_env_opencl_embedded_1_2:
      return SPV_ENV_OPENCL_EMBEDDED_1_2;
    case shaderc_spvc_spv_env_opencl_2_0:
      return SPV_ENV_OPENCL_2_0;
    case shaderc_spvc_spv_env_opencl_embedded_2_0:
      return SPV_ENV_OPENCL_EMBEDDED_2_0;
    case shaderc_spvc_spv_env_opencl_embedded_2_1:
      return SPV_ENV_OPENCL_EMBEDDED_2_1;
    case shaderc_spvc_spv_env_opencl_embedded_2_2:
      return SPV_ENV_OPENCL_EMBEDDED_2_2;
    case shaderc_spvc_spv_env_universal_1_3:
      return SPV_ENV_UNIVERSAL_1_3;
    case shaderc_spvc_spv_env_vulkan_1_1:
      return SPV_ENV_VULKAN_1_1;
    case shaderc_spvc_spv_env_webgpu_0:
      return SPV_ENV_WEBGPU_0;
    case shaderc_spvc_spv_env_universal_1_4:
      return SPV_ENV_UNIVERSAL_1_4;
    case shaderc_spvc_spv_env_vulkan_1_1_spirv_1_4:
      return SPV_ENV_VULKAN_1_1_SPIRV_1_4;
    case shaderc_spvc_spv_env_universal_1_5:
      return SPV_ENV_UNIVERSAL_1_5;
    case shaderc_spvc_spv_env_vulkan_1_2:
      return SPV_ENV_VULKAN_1_2;
  }
  shaderc_spvc::ErrorLog(nullptr)
      << "Attempted to convert unknown shaderc_spvc_spv_env value, " << env;
  assert(false);
  return SPV_ENV_UNIVERSAL_1_0;
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
  }
  context->messages.clear();
  return context->messages_string.c_str();
}

shaderc_spvc_status shaderc_spvc_context_get_compiler(
    const shaderc_spvc_context_t context, void** compiler) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  CHECK_OUT_PARAM(context, compiler, "compiler");

  *compiler = context->cross_compiler.get();
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_context_set_use_spvc_parser(
    shaderc_spvc_context_t context, bool b) {
  CHECK_CONTEXT(context);

  context->use_spvc_parser = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_compile_options_t shaderc_spvc_compile_options_create(
    shaderc_spvc_spv_env source_env, shaderc_spvc_spv_env target_env) {
  shaderc_spvc_compile_options_t options =
      new (std::nothrow) shaderc_spvc_compile_options;
  if (options) {
    options->glsl.version = 0;
    options->source_env = shaderc_spvc_spv_env_to_spv_target_env(source_env);
    options->target_env = shaderc_spvc_spv_env_to_spv_target_env(target_env);
  }
  return options;
}

shaderc_spvc_compile_options_t shaderc_spvc_compile_options_clone(
    shaderc_spvc_compile_options_t options) {
  if (options) return new (std::nothrow) shaderc_spvc_compile_options(*options);
  return nullptr;
}

void shaderc_spvc_compile_options_destroy(
    shaderc_spvc_compile_options_t options) {
  if (options) delete options;
}

// DEPRECATED
shaderc_spvc_status shaderc_spvc_compile_options_set_source_env(
    shaderc_spvc_compile_options_t options, shaderc_target_env env,
    shaderc_env_version version) {
  CHECK_OPTIONS(nullptr, options);

  options->source_env = spvc_private::get_spv_target_env(env, version);
  return shaderc_spvc_status_success;
}

// DEPRECATED
shaderc_spvc_status shaderc_spvc_compile_options_set_target_env(
    shaderc_spvc_compile_options_t options, shaderc_target_env env,
    shaderc_env_version version) {
  CHECK_OPTIONS(nullptr, options);

  options->target_env = spvc_private::get_spv_target_env(env, version);
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_entry_point(
    shaderc_spvc_compile_options_t options, const char* entry_point) {
  CHECK_OPTIONS(nullptr, options);
  CHECK_IN_PARAM(nullptr, entry_point, "entry_point");

  options->entry_point = entry_point;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_remove_unused_variables(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->remove_unused_variables = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_robust_buffer_access_pass(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->robust_buffer_access_pass = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_emit_line_directives(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->glsl.emit_line_directives = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_vulkan_semantics(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->glsl.vulkan_semantics = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_separate_shader_objects(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->glsl.separate_shader_objects = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_flatten_ubo(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->flatten_ubo = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_glsl_language_version(
    shaderc_spvc_compile_options_t options, uint32_t version) {
  CHECK_OPTIONS(nullptr, options);

  options->glsl.version = version;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status
shaderc_spvc_compile_options_set_flatten_multidimensional_arrays(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->glsl.flatten_multidimensional_arrays = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status
shaderc_spvc_compile_options_set_force_zero_initialized_variables(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->glsl.force_zero_initialized_variables = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_es(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->forced_es_setting = b;
  options->force_es = true;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status
shaderc_spvc_compile_options_set_glsl_emit_push_constant_as_ubo(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->glsl.emit_push_constant_as_uniform_buffer = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_msl_language_version(
    shaderc_spvc_compile_options_t options, uint32_t version) {
  CHECK_OPTIONS(nullptr, options);

  options->msl.msl_version = version;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status
shaderc_spvc_compile_options_set_msl_swizzle_texture_samples(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->msl.swizzle_texture_samples = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_msl_platform(
    shaderc_spvc_compile_options_t options,
    shaderc_spvc_msl_platform platform) {
  CHECK_OPTIONS(nullptr, options);

  switch (platform) {
    case shaderc_spvc_msl_platform_ios:
      options->msl.platform = spirv_cross::CompilerMSL::Options::iOS;
      break;
    case shaderc_spvc_msl_platform_macos:
      options->msl.platform = spirv_cross::CompilerMSL::Options::macOS;
      break;
  }
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_msl_pad_fragment_output(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->msl.pad_fragment_output_components = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_msl_capture(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->msl.capture_output_to_buffer = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_msl_domain_lower_left(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->msl.tess_domain_origin_lower_left = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_msl_argument_buffers(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->msl.argument_buffers = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status
shaderc_spvc_compile_options_set_msl_discrete_descriptor_sets(
    shaderc_spvc_compile_options_t options, const uint32_t* descriptors,
    size_t num_descriptors) {
  CHECK_OPTIONS(nullptr, options);

  options->msl_discrete_descriptor_sets.resize(num_descriptors);
  std::copy_n(descriptors, num_descriptors,
              options->msl_discrete_descriptor_sets.begin());
  return shaderc_spvc_status_success;
}

shaderc_spvc_status
shaderc_spvc_compile_options_set_msl_enable_point_size_builtin(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->msl.enable_point_size_builtin = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status
shaderc_spvc_compile_options_set_msl_buffer_size_buffer_index(
    shaderc_spvc_compile_options_t options, uint32_t index) {
  CHECK_OPTIONS(nullptr, options);

  options->msl.buffer_size_buffer_index = index;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_hlsl_shader_model(
    shaderc_spvc_compile_options_t options, uint32_t model) {
  CHECK_OPTIONS(nullptr, options);

  options->hlsl.shader_model = model;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_hlsl_point_size_compat(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->hlsl.point_size_compat = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_hlsl_point_coord_compat(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->hlsl.point_coord_compat = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_fixup_clipspace(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->glsl.vertex.fixup_clipspace = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_flip_vert_y(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->glsl.vertex.flip_vert_y = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_validate(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->validate = b;
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_compile_options_set_optimize(
    shaderc_spvc_compile_options_t options, bool b) {
  CHECK_OPTIONS(nullptr, options);

  options->optimize = b;
  return shaderc_spvc_status_success;
}

size_t shaderc_spvc_compile_options_set_for_fuzzing(
    shaderc_spvc_compile_options_t options, const uint8_t* data, size_t size) {
  if (!options || !data || size < sizeof(*options)) return 0;

  memcpy(static_cast<void*>(options), data, sizeof(*options));
  return sizeof(*options);
}

shaderc_spvc_status shaderc_spvc_initialize_impl(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options,
    shaderc_spvc_status (*generator)(const shaderc_spvc_context_t,
                                     const uint32_t*, size_t,
                                     shaderc_spvc_compile_options_t)) {
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
  CHECK_CONTEXT(context);
  CHECK_OPTIONS(context, options);
  CHECK_IN_PARAM(context, source, "source");

  context->target_lang = SPVC_TARGET_LANG_GLSL;
  return shaderc_spvc_initialize_impl(context, source, source_len, options,
                                      spvc_private::generate_glsl_compiler);
}

shaderc_spvc_status shaderc_spvc_initialize_for_hlsl(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {
  CHECK_CONTEXT(context);
  CHECK_OPTIONS(context, options);
  CHECK_IN_PARAM(context, source, "source");

  context->target_lang = SPVC_TARGET_LANG_HLSL;
  return shaderc_spvc_initialize_impl(context, source, source_len, options,
                                      spvc_private::generate_hlsl_compiler);
}

shaderc_spvc_status shaderc_spvc_initialize_for_msl(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {
  CHECK_CONTEXT(context);
  CHECK_OPTIONS(context, options);
  CHECK_IN_PARAM(context, source, "source");

  context->target_lang = SPVC_TARGET_LANG_MSL;
  return shaderc_spvc_initialize_impl(context, source, source_len, options,
                                      spvc_private::generate_msl_compiler);
}

shaderc_spvc_status shaderc_spvc_initialize_for_vulkan(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options) {
  CHECK_CONTEXT(context);
  CHECK_OPTIONS(context, options);
  CHECK_IN_PARAM(context, source, "source");

  context->target_lang = SPVC_TARGET_LANG_VULKAN;
  return shaderc_spvc_initialize_impl(context, source, source_len, options,
                                      spvc_private::generate_vulkan_compiler);
}

shaderc_spvc_status shaderc_spvc_compile_shader(
    const shaderc_spvc_context_t context,
    shaderc_spvc_compilation_result_t result) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);

  if (context->target_lang == SPVC_TARGET_LANG_UNKNOWN) {
    shaderc_spvc::ErrorLog(context)
        << "Invoked compile_shader with unknown language";
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
      shaderc_spvc::ErrorLog(context) << "Compilation failed.  Partial source:";
      if (context->target_lang == SPVC_TARGET_LANG_GLSL) {
        spirv_cross::CompilerGLSL* cast_compiler =
            reinterpret_cast<spirv_cross::CompilerGLSL*>(
                context->cross_compiler.get());
        shaderc_spvc::ErrorLog(context) << cast_compiler->get_partial_source();
      } else if (context->target_lang == SPVC_TARGET_LANG_HLSL) {
        spirv_cross::CompilerHLSL* cast_compiler =
            reinterpret_cast<spirv_cross::CompilerHLSL*>(
                context->cross_compiler.get());
        shaderc_spvc::ErrorLog(context) << cast_compiler->get_partial_source();
      } else if (context->target_lang == SPVC_TARGET_LANG_MSL) {
        spirv_cross::CompilerMSL* cast_compiler =
            reinterpret_cast<spirv_cross::CompilerMSL*>(
                context->cross_compiler.get());
        shaderc_spvc::ErrorLog(context) << cast_compiler->get_partial_source();
      } else {
        shaderc_spvc::ErrorLog(context)
            << "Unexpected target language in context";
      }
      context->cross_compiler.reset();
    }
    return status;
  }
}

shaderc_spvc_status shaderc_spvc_set_decoration(
    const shaderc_spvc_context_t context, uint32_t id,
    shaderc_spvc_decoration decoration, uint32_t argument) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);

  spv::Decoration spirv_cross_decoration;
  shaderc_spvc_status status =
      spvc_private::shaderc_spvc_decoration_to_spirv_cross_decoration(
          decoration, &spirv_cross_decoration);
  if (status == shaderc_spvc_status_success) {
    context->cross_compiler->set_decoration(static_cast<spirv_cross::ID>(id),
                                            spirv_cross_decoration, argument);
  } else {
    shaderc_spvc::ErrorLog(context) << "Decoration Conversion failed. "
                                       "shaderc_spvc_decoration not supported.";
  }
  return status;
}

shaderc_spvc_status shaderc_spvc_get_decoration(
    const shaderc_spvc_context_t context, uint32_t id,
    shaderc_spvc_decoration decoration, uint32_t* value) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  CHECK_OUT_PARAM(context, value, "value");

  spv::Decoration spirv_cross_decoration;
  shaderc_spvc_status status =
      spvc_private::shaderc_spvc_decoration_to_spirv_cross_decoration(
          decoration, &spirv_cross_decoration);
  if (status != shaderc_spvc_status_success) {
    shaderc_spvc::ErrorLog(context) << "Decoration conversion failed. "
                                       "shaderc_spvc_decoration not supported.";

    return status;
  }

  *value = context->cross_compiler->get_decoration(
      static_cast<spirv_cross::ID>(id), spirv_cross_decoration);
  if (*value == 0) {
    shaderc_spvc::ErrorLog(context)
        << "Getting decoration failed. id not found.";
    return shaderc_spvc_status_compilation_error;
  }

  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_unset_decoration(
    const shaderc_spvc_context_t context, uint32_t id,
    shaderc_spvc_decoration decoration) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);

  spv::Decoration spirv_cross_decoration;
  shaderc_spvc_status status =
      spvc_private::shaderc_spvc_decoration_to_spirv_cross_decoration(
          decoration, &spirv_cross_decoration);
  if (status == shaderc_spvc_status_success) {
    context->cross_compiler->unset_decoration(static_cast<spirv_cross::ID>(id),
                                              spirv_cross_decoration);
  } else {
    shaderc_spvc::ErrorLog(context) << "Decoration conversion failed. "
                                       "shaderc_spvc_decoration not supported.";
  }

  return status;
}

shaderc_spvc_status shaderc_spvc_get_combined_image_samplers(
    const shaderc_spvc_context_t context,
    shaderc_spvc_combined_image_sampler* samplers, size_t* num_samplers) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  CHECK_OUT_PARAM(context, num_samplers, "num_samplers");

  *num_samplers = context->cross_compiler->get_combined_image_samplers().size();
  if (!samplers) return shaderc_spvc_status_success;

  for (const auto& combined :
       context->cross_compiler->get_combined_image_samplers()) {
    samplers->combined_id = combined.combined_id;
    samplers->image_id = combined.image_id;
    samplers->sampler_id = combined.sampler_id;
    samplers++;
  }
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_set_name(const shaderc_spvc_context_t context,
                                          uint32_t id, const char* name) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  CHECK_IN_PARAM(context, name, "name");

  context->cross_compiler->set_name(id, name);
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_add_msl_resource_binding(
    const shaderc_spvc_context_t context,
    const shaderc_spvc_msl_resource_binding binding) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  if (context->target_lang != SPVC_TARGET_LANG_MSL) {
    shaderc_spvc::ErrorLog(context)
        << "Invoked add_msl_resource_binding when target language was not MSL";
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
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  CHECK_IN_PARAM(context, function_name, "function_name");
  CHECK_OUT_PARAM(context, workgroup_size, "workgroup_size");

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
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  CHECK_OUT_PARAM(context, b, "b");
  if (context->target_lang != SPVC_TARGET_LANG_MSL) {
    shaderc_spvc::ErrorLog(context)
        << "Invoked needs_buffer_size_buffer when target language was not MSL";
    return shaderc_spvc_status_configuration_error;
  }

  *b =
      reinterpret_cast<spirv_cross::CompilerMSL*>(context->cross_compiler.get())
          ->needs_buffer_size_buffer();
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_build_combined_image_samplers(
    const shaderc_spvc_context_t context) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);

  context->cross_compiler->build_combined_image_samplers();
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_get_execution_model(
    const shaderc_spvc_context_t context,
    shaderc_spvc_execution_model* execution_model) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  CHECK_OUT_PARAM(context, execution_model, "execution_model");

  auto spirv_model = context->cross_compiler->get_execution_model();
  *execution_model = spv_model_to_spvc_model(spirv_model);
  if (*execution_model == shaderc_spvc_execution_model_invalid) {
    shaderc_spvc::ErrorLog(context)
        << "Shader execution model appears to be of an unsupported type";
    return shaderc_spvc_status_internal_error;
  }

  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_get_push_constant_buffer_count(
    const shaderc_spvc_context_t context, size_t* count) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  CHECK_OUT_PARAM(context, count, "count");

  *count = context->cross_compiler->get_shader_resources()
               .push_constant_buffers.size();
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_get_binding_info(
    const shaderc_spvc_context_t context, shaderc_spvc_shader_resource resource,
    shaderc_spvc_binding_type binding_type, shaderc_spvc_binding_info* bindings,
    size_t* binding_count) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  CHECK_OUT_PARAM(context, binding_count, "binding_count");

  auto* compiler = context->cross_compiler.get();
  const auto& resources = compiler->get_shader_resources();
  const auto* shader_resources = get_shader_resources(resources, resource);
  *binding_count = shader_resources->size();
  if (!bindings) return shaderc_spvc_status_success;

  for (const auto& shader_resource : *shader_resources) {
    bindings->texture_dimension = shaderc_spvc_texture_view_dimension_undefined;
    bindings->texture_component_type = shaderc_spvc_texture_format_type_float;

    if (!compiler->get_decoration_bitset(shader_resource.id)
             .get(spv::DecorationBinding)) {
      shaderc_spvc::ErrorLog(context)
          << "Unable to get binding decoration for shader resource";
      return shaderc_spvc_status_internal_error;
    }
    uint32_t binding_decoration =
        compiler->get_decoration(shader_resource.id, spv::DecorationBinding);
    bindings->binding = binding_decoration;

    if (!compiler->get_decoration_bitset(shader_resource.id)
             .get(spv::DecorationDescriptorSet)) {
      shaderc_spvc::ErrorLog(context)
          << "Unable to get descriptor set decoration for shader resource";
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
      case shaderc_spvc_binding_type_storage_texture: {
        spirv_cross::Bitset flags = compiler->get_decoration_bitset(shader_resource.id);
        if (flags.get(spv::DecorationNonReadable)) {
          bindings->binding_type = shaderc_spvc_binding_type_writeonly_storage_texture;
        } else if (flags.get(spv::DecorationNonWritable)) {
            bindings->binding_type = shaderc_spvc_binding_type_readonly_storage_texture;
        } else {
            bindings->binding_type = shaderc_spvc_binding_type_storage_texture;
        }
        spirv_cross::SPIRType::ImageType imageType =
            compiler->get_type(bindings->base_type_id).image;
        bindings->storage_texture_format =
            spv_image_format_to_storage_texture_format(imageType.format);
        bindings->texture_dimension = spirv_dim_to_texture_view_dimension(
            imageType.dim, imageType.arrayed);
        bindings->multisampled = imageType.ms;
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
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  CHECK_OUT_PARAM(context, location_count, "location_count");

  auto* compiler = context->cross_compiler.get();
  shaderc_spvc_status status = get_location_info_impl(
      compiler, compiler->get_shader_resources().stage_inputs, locations,
      location_count);
  if (status != shaderc_spvc_status_success) {
    shaderc_spvc::ErrorLog(context)
        << "Unable to get location decoration for stage input";
  }

  return status;
}

shaderc_spvc_status shaderc_spvc_get_output_stage_location_info(
    const shaderc_spvc_context_t context,
    shaderc_spvc_resource_location_info* locations, size_t* location_count) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  CHECK_OUT_PARAM(context, location_count, "location_count");

  auto* compiler = context->cross_compiler.get();
  shaderc_spvc_status status = get_location_info_impl(
      compiler, compiler->get_shader_resources().stage_outputs, locations,
      location_count);
  if (status != shaderc_spvc_status_success) {
    shaderc_spvc::ErrorLog(context)
        << "Unable to get location decoration for stage output";
  }

  return status;
}

shaderc_spvc_status shaderc_spvc_get_output_stage_type_info(
    const shaderc_spvc_context_t context,
    shaderc_spvc_resource_type_info* types, size_t* type_count) {
  CHECK_CONTEXT(context);
  CHECK_CROSS_COMPILER(context, context->cross_compiler);
  CHECK_OUT_PARAM(context, type_count, "type_count");

  auto* compiler = context->cross_compiler.get();
  const auto& resources = compiler->get_shader_resources().stage_outputs;

  *type_count = resources.size();
  if (!types) return shaderc_spvc_status_success;

  for (const auto& resource : resources) {
    if (!compiler->get_decoration_bitset(resource.id)
             .get(spv::DecorationLocation)) {
      shaderc_spvc::ErrorLog(context)
          << "Unable to get location decoration for stage output";
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

shaderc_spvc_status shaderc_spvc_result_get_string_output(
    const shaderc_spvc_compilation_result_t result, const char** str) {
  CHECK_RESULT(nullptr, result);
  CHECK_OUT_PARAM(nullptr, str, "str");

  *str = result->string_output.c_str();
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_result_get_binary_output(
    const shaderc_spvc_compilation_result_t result,
    const uint32_t** binary_output) {
  CHECK_RESULT(nullptr, result);
  CHECK_OUT_PARAM(nullptr, binary_output, "binary_output");

  *binary_output = result->binary_output.data();
  return shaderc_spvc_status_success;
}

shaderc_spvc_status shaderc_spvc_result_get_binary_length(
    const shaderc_spvc_compilation_result_t result, uint32_t* len) {
  CHECK_RESULT(nullptr, result);
  CHECK_OUT_PARAM(nullptr, len, "len");

  *len = result->binary_output.size();
  return shaderc_spvc_status_success;
}
