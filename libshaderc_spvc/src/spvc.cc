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

shaderc_spvc_context_t shaderc_spvc_context_create() {
  return new (std::nothrow) shaderc_spvc_context;
}

void shaderc_spvc_context_destroy(shaderc_spvc_context_t context) {
  if (context) delete context;
}

const char* shaderc_spvc_context_get_messages(
    const shaderc_spvc_context_t context) {
  return context->messages.c_str();
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
      context->messages.append("Compilation failed.  Partial source:\n");
      if (context->target_lang == SPVC_TARGET_LANG_GLSL) {
        spirv_cross::CompilerGLSL* cast_compiler =
            reinterpret_cast<spirv_cross::CompilerGLSL*>(
                context->cross_compiler.get());
        context->messages.append(cast_compiler->get_partial_source());
      } else if (context->target_lang == SPVC_TARGET_LANG_HLSL) {
        spirv_cross::CompilerHLSL* cast_compiler =
            reinterpret_cast<spirv_cross::CompilerHLSL*>(
                context->cross_compiler.get());
        context->messages.append(cast_compiler->get_partial_source());
      } else if (context->target_lang == SPVC_TARGET_LANG_MSL) {
        spirv_cross::CompilerMSL* cast_compiler =
            reinterpret_cast<spirv_cross::CompilerMSL*>(
                context->cross_compiler.get());
        context->messages.append(cast_compiler->get_partial_source());
      } else {
        context->messages.append("Unexpected target language in context\n");
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
    context->messages.append(
        "Decoration Conversion failed.  shaderc_spvc_decoration not "
        "supported.\n ");
  }
  return status;
}

shaderc_spvc_status shaderc_spvc_get_decoration(
    const shaderc_spvc_context_t context, uint32_t id,
    shaderc_spvc_decoration decoration, uint32_t* argument_ptr) {
  spv::Decoration spirv_cross_decoration;
  shaderc_spvc_status status =
      spvc_private::shaderc_spvc_decoration_to_spirv_cross_decoration(
          decoration, &spirv_cross_decoration);
  if (argument_ptr && status == shaderc_spvc_status_success) {
    *argument_ptr = context->cross_compiler->get_decoration(
        static_cast<spirv_cross::ID>(id), spirv_cross_decoration);
    if (*argument_ptr == 0) {
      status = shaderc_spvc_status_compilation_error;
      context->messages.append("Getting decoration failed. id not found. \n ");
    }
  } else {
    context->messages.append(
        "Decoration conversion failed.  shaderc_spvc_decoration not "
        "supported.\n ");
  }
  return status;
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
    context->messages.append(
        "Decoration conversion failed.  shaderc_spvc_decoration not "
        "supported.\n ");
  }
  return status;
}

void shaderc_spvc_for_each_combined_image_sampler(
    const shaderc_spvc_context_t context,
    void (*f)(uint32_t, uint32_t, uint32_t)) {
  for (const auto& combined :
       context->cross_compiler->get_combined_image_samplers()) {
    f(combined.sampler_id, combined.image_id, combined.combined_id);
  }
}

void shaderc_spvc_set_name(const shaderc_spvc_context_t context, uint32_t id,
                           const char* name) {
  context->cross_compiler->set_name(id, name);
  return;
}

void shaderc_spvc_build_combined_image_samplers(
    const shaderc_spvc_context_t context) {
  context->cross_compiler->build_combined_image_samplers();
  return;
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

namespace shaderc_spvc {

void Context::ForEachCombinedImageSamplers(
    std::function<void(uint32_t, uint32_t, uint32_t)> f) {
  for (const auto& combined :
       context_->cross_compiler->get_combined_image_samplers()) {
    f(combined.sampler_id, combined.image_id, combined.combined_id);
  }
}

}  // namespace shaderc_spvc
