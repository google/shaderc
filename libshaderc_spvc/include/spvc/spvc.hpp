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

#ifndef SHADERC_SPVC_HPP_
#define SHADERC_SPVC_HPP_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "spvc.h"

namespace shaderc_spvc {

// A CompilationResult contains the compiler output, compilation status,
// and messages.
//
// The compiler output is stored as an array of elements and accessed
// via random access iterators provided by cbegin() and cend().  The iterators
// are contiguous in the sense of "Contiguous Iterators: A Refinement of
// Random Access Iterators", Nevin Liber, C++ Library Evolution Working
// Group Working Paper N3884.
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3884.pdf
//
// Methods begin() and end() are also provided to enable range-based for.
// They are synonyms to cbegin() and cend(), respectively.
class CompilationResult {
 public:
  // Upon creation, the CompilationResult takes ownership of the
  // shaderc_spvc_compilation_result instance. During destruction of the
  // CompilationResult, the shaderc_spvc_compilation_result will be released.
  explicit CompilationResult()
      : result_(shaderc_spvc_result_create(), shaderc_spvc_result_destroy) {}
  ~CompilationResult() {}

  CompilationResult(CompilationResult&& other)
      : result_(nullptr, shaderc_spvc_result_destroy) {
    *this = std::move(other);
  }

  CompilationResult& operator=(CompilationResult&& other) {
    result_.reset(other.result_.release());
    return *this;
  }

  shaderc_spvc_status GetStringOutput(std::string* str) const {
    if (!str) return shaderc_spvc_status_invalid_out_param;
    const char* inner_str;
    shaderc_spvc_status status =
        shaderc_spvc_result_get_string_output(result_.get(), &inner_str);
    if (status != shaderc_spvc_status_success) {
      return status;
    }
    *str = std::string(inner_str);
    return shaderc_spvc_status_success;
  }

  shaderc_spvc_status GetBinaryOutput(std::vector<uint32_t>* data) const {
    if (!data) return shaderc_spvc_status_invalid_out_param;
    const uint32_t* binary_output;
    shaderc_spvc_status status =
        shaderc_spvc_result_get_binary_output(result_.get(), &binary_output);
    if (status != shaderc_spvc_status_success) {
      return status;
    }
    uint32_t binary_length;
    status =
        shaderc_spvc_result_get_binary_length(result_.get(), &binary_length);
    if (!binary_output || !binary_length) {
      *data = std::vector<uint32_t>();
    } else {
      *data =
          std::vector<uint32_t>(binary_output, binary_output + binary_length);
    }
    return shaderc_spvc_status_success;
  }

 private:
  friend class Context;

  CompilationResult(const CompilationResult& other) = delete;
  CompilationResult& operator=(const CompilationResult& other) = delete;

  std::unique_ptr<shaderc_spvc_compilation_result,
                  void (*)(shaderc_spvc_compilation_result_t)>
      result_;
};

// Contains any options that can have default values for a compilation.
class CompileOptions {
 public:
  CompileOptions(shaderc_spvc_spv_env source_env,
                 shaderc_spvc_spv_env target_env)
      : options_(shaderc_spvc_compile_options_create(source_env, target_env),
                 shaderc_spvc_compile_options_destroy) {}
  // DEPRECATED
  CompileOptions()
      : CompileOptions(shaderc_spvc_spv_env_universal_1_0,
                       shaderc_spvc_spv_env_universal_1_0) {}
  CompileOptions(const CompileOptions& other)
      : options_(nullptr, shaderc_spvc_compile_options_destroy) {
    options_.reset(shaderc_spvc_compile_options_clone(other.options_.get()));
  }

  CompileOptions(CompileOptions&& other)
      : options_(nullptr, shaderc_spvc_compile_options_destroy) {
    options_.reset(other.options_.release());
  }

  // DEPRECATED
  // Set the environment for the input SPIR-V.  Default is Vulkan 1.0.
  shaderc_spvc_status SetSourceEnvironment(shaderc_target_env env,
                                           shaderc_env_version version) {
    return shaderc_spvc_compile_options_set_source_env(options_.get(), env,
                                                       version);
  }

  // DEPRECATED
  // Set the target environment for the SPIR-V to be cross-compiled. If this is
  // different then the source a transformation will need to be applied.
  // Currently only Vulkan 1.1 <-> WebGPU transforms are defined. Default is
  // Vulkan 1.0.
  shaderc_spvc_status SetTargetEnvironment(shaderc_target_env env,
                                           shaderc_env_version version) {
    return shaderc_spvc_compile_options_set_target_env(options_.get(), env,
                                                       version);
  }

  // Set the entry point.
  shaderc_spvc_status SetEntryPoint(const std::string& entry_point) {
    return shaderc_spvc_compile_options_set_entry_point(options_.get(),
                                                        entry_point.c_str());
  }

  // If true, unused variables will not appear in the output.
  shaderc_spvc_status SetRemoveUnusedVariables(bool b) {
    return shaderc_spvc_compile_options_set_remove_unused_variables(
        options_.get(), b);
  }

  // If true, enable robust buffer access pass in the spirv-opt, meaning:
  // Inject code to clamp indexed accesses to buffers and internal
  // arrays, providing guarantees satisfying Vulkan's robustBufferAccess rules.
  // This is useful when an implementation does not support robust-buffer access
  // as a driver option.
  shaderc_spvc_status SetRobustBufferAccessPass(bool b) {
    return shaderc_spvc_compile_options_set_robust_buffer_access_pass(
        options_.get(), b);
  }

  shaderc_spvc_status SetEmitLineDirectives(bool b) {
    return shaderc_spvc_compile_options_set_emit_line_directives(options_.get(),
                                                                 b);
  }
  // If true, Vulkan GLSL features are used instead of GL-compatible features.
  shaderc_spvc_status SetVulkanSemantics(bool b) {
    return shaderc_spvc_compile_options_set_vulkan_semantics(options_.get(), b);
  }

  // If true, gl_PerVertex is explicitly redeclared in vertex, geometry and
  // tessellation shaders. The members of gl_PerVertex is determined by which
  // built-ins are declared by the shader.
  shaderc_spvc_status SetSeparateShaderObjects(bool b) {
    return shaderc_spvc_compile_options_set_separate_shader_objects(
        options_.get(), b);
  }

  // Flatten uniform or push constant variable into (i|u)vec4 array.
  shaderc_spvc_status SetFlattenUbo(bool b) {
    return shaderc_spvc_compile_options_set_flatten_ubo(options_.get(), b);
  }

  // Which GLSL version should be produced.  Default is 450 (i.e. 4.5).
  shaderc_spvc_status SetGLSLLanguageVersion(uint32_t version) {
    return shaderc_spvc_compile_options_set_glsl_language_version(
        options_.get(), version);
  }

  // If true, flatten multidimensional arrays, e.g. foo[a][b][c] -> foo[a*b*c].
  // Default is false.
  shaderc_spvc_status SetFlattenMultidimensionalArrays(bool b) {
    return shaderc_spvc_compile_options_set_flatten_multidimensional_arrays(
        options_.get(), b);
  }

  // If true, initialize new variables from cross-compile to 0 if possible.
  // Default is false.
  shaderc_spvc_status SetForceZeroInitializedVariables(bool b) {
    return shaderc_spvc_compile_options_set_force_zero_initialized_variables(
        options_.get(), b);
  }

  // Force interpretion as ES, or not.  Default is to detect from source.
  shaderc_spvc_status SetES(bool b) {
    return shaderc_spvc_compile_options_set_es(options_.get(), b);
  }

  // If true, emit push constants as uniform buffer objects.  Default is false.
  shaderc_spvc_status SetGLSLEmitPushConstantAsUBO(bool b) {
    return shaderc_spvc_compile_options_set_glsl_emit_push_constant_as_ubo(
        options_.get(), b);
  }

  // Which MSL version should be produced.  Default is 10200 (i.e. 1.2).
  shaderc_spvc_status SetMSLLanguageVersion(uint32_t version) {
    return shaderc_spvc_compile_options_set_msl_language_version(options_.get(),
                                                                 version);
  }

  // If true, swizzle MSL texture samples.  Default is false.
  shaderc_spvc_status SetMSLSwizzleTextureSamples(bool b) {
    return shaderc_spvc_compile_options_set_msl_swizzle_texture_samples(
        options_.get(), b);
  }

  // Choose MSL platform.  Default is MacOS.
  shaderc_spvc_status SetMSLPlatform(shaderc_spvc_msl_platform platform) {
    return shaderc_spvc_compile_options_set_msl_platform(options_.get(),
                                                         platform);
  }

  // If true, pad MSL fragment output.  Default is false.
  shaderc_spvc_status SetMSLPadFragmentOutput(bool b) {
    return shaderc_spvc_compile_options_set_msl_pad_fragment_output(
        options_.get(), b);
  }

  // If true, capture MSL output to buffer.  Default is false.
  shaderc_spvc_status SetMSLCapture(bool b) {
    return shaderc_spvc_compile_options_set_msl_capture(options_.get(), b);
  }

  // If true, flip the Y-coord of the built-in "TessCoord."  Default is top
  // left.
  shaderc_spvc_status SetMSLDomainLowerLeft(bool b) {
    return shaderc_spvc_compile_options_set_msl_domain_lower_left(
        options_.get(), b);
  }

  // Enable use of MSL 2.0 indirect argument buffers.  Default is not to use
  // them.
  shaderc_spvc_status SetMSLArgumentBuffers(bool b) {
    return shaderc_spvc_compile_options_set_msl_argument_buffers(options_.get(),
                                                                 b);
  }

  // When using MSL argument buffers, force "classic" MSL 1.0 binding for the
  // given descriptor sets. This corresponds to VK_KHR_push_descriptor in
  // Vulkan.
  shaderc_spvc_status SetMSLDiscreteDescriptorSets(
      const std::vector<uint32_t> descriptors) {
    return shaderc_spvc_compile_options_set_msl_discrete_descriptor_sets(
        options_.get(), descriptors.data(), descriptors.size());
  }

  // Set whether or not PointSize builtin is used for MSL shaders
  shaderc_spvc_status SetMSLEnablePointSizeBuiltIn(bool b) {
    return shaderc_spvc_compile_options_set_msl_enable_point_size_builtin(
        options_.get(), b);
  }

  // Set the index in the buffer size in the buffer for MSL
  shaderc_spvc_status SetMSLBufferSizeBufferIndex(uint32_t index) {
    return shaderc_spvc_compile_options_set_msl_buffer_size_buffer_index(
        options_.get(), index);
  }

  // Which HLSL shader model should be used.  Default is 30.
  shaderc_spvc_status SetHLSLShaderModel(uint32_t model) {
    return shaderc_spvc_compile_options_set_hlsl_shader_model(options_.get(),
                                                              model);
  }

  // If true, ignore PointSize.  Default is false.
  shaderc_spvc_status SetHLSLPointSizeCompat(bool b) {
    return shaderc_spvc_compile_options_set_hlsl_point_size_compat(
        options_.get(), b);
  }

  // If true, ignore PointCoord.  Default is false.
  shaderc_spvc_status SetHLSLPointCoordCompat(bool b) {
    return shaderc_spvc_compile_options_set_hlsl_point_coord_compat(
        options_.get(), b);
  }

  // If true (default is false):
  //   GLSL: map depth from Vulkan/D3D style to GL style, i.e. [ 0,w] -> [-w,w]
  //   MSL : map depth from GL style to Vulkan/D3D style, i.e. [-w,w] -> [ 0,w]
  //   HLSL: map depth from GL style to Vulkan/D3D style, i.e. [-w,w] -> [ 0,w]
  shaderc_spvc_status SetFixupClipspace(bool b) {
    return shaderc_spvc_compile_options_set_fixup_clipspace(options_.get(), b);
  }

  // If true invert gl_Position.y or equivalent.  Default is false.
  shaderc_spvc_status SetFlipVertY(bool b) {
    return shaderc_spvc_compile_options_set_flip_vert_y(options_.get(), b);
  }

  // If true validate input and intermediate source. Default is true.
  shaderc_spvc_status SetValidate(bool b) {
    return shaderc_spvc_compile_options_set_validate(options_.get(), b);
  }

  // If true optimize input and intermediate source. Default is true.
  shaderc_spvc_status SetOptimize(bool b) {
    return shaderc_spvc_compile_options_set_optimize(options_.get(), b);
  }

  // Fill options with given data.  Return amount of data used, or zero
  // if not enough data was given.
  size_t SetForFuzzing(const uint8_t* data, size_t size) {
    return shaderc_spvc_compile_options_set_for_fuzzing(options_.get(), data,
                                                        size);
  }

 private:
  CompileOptions& operator=(const CompileOptions& other) = delete;
  std::unique_ptr<shaderc_spvc_compile_options,
                  void (*)(shaderc_spvc_compile_options_t)>
      options_;

  friend class Context;
};

// The compilation context for compiling SPIR-V.
class Context {
 public:
  Context()
      : context_(shaderc_spvc_context_create(), shaderc_spvc_context_destroy) {}

  Context(Context&& other) : context_(nullptr, shaderc_spvc_context_destroy) {
    context_.reset(other.context_.release());
  }

  bool IsValid() const { return context_ != nullptr; }

  // Returns logged messages from operations
  const std::string GetMessages() const {
    return shaderc_spvc_context_get_messages(context_.get());
  }

  // EXPERIMENTAL
  // Returns the internal spirv_cross compiler reference, does NOT transfer
  // ownership.
  // This is being exposed temporarily to ease integration of spvc into Dawn,
  // but this is will be removed in the future without warning.
  shaderc_spvc_status GetCompiler(void** compiler) {
    return shaderc_spvc_context_get_compiler(context_.get(), compiler);
  }

  shaderc_spvc_status SetUseSpvcParser(bool b) {
    return shaderc_spvc_context_set_use_spvc_parser(context_.get(), b);
  }

  // Initializes state for compiling SPIR-V to GLSL.
  shaderc_spvc_status InitializeForGlsl(const uint32_t* source,
                                        size_t source_len,
                                        const CompileOptions& options) const {
    return shaderc_spvc_initialize_for_glsl(context_.get(), source, source_len,
                                            options.options_.get());
  }

  // Initializes state for compiling SPIR-V to HLSL.
  shaderc_spvc_status InitializeForHlsl(const uint32_t* source,
                                        size_t source_len,
                                        const CompileOptions& options) const {
    return shaderc_spvc_initialize_for_hlsl(context_.get(), source, source_len,
                                            options.options_.get());
  }

  // Initializes state for compiling SPIR-V to MSL.
  shaderc_spvc_status InitializeForMsl(const uint32_t* source,
                                       size_t source_len,
                                       const CompileOptions& options) const {
    return shaderc_spvc_initialize_for_msl(context_.get(), source, source_len,
                                           options.options_.get());
  }

  // Initializes state for compiling SPIR-V to Vulkan.
  shaderc_spvc_status InitializeForVulkan(const uint32_t* source,
                                          size_t source_len,
                                          const CompileOptions& options) const {
    return shaderc_spvc_initialize_for_vulkan(
        context_.get(), source, source_len, options.options_.get());
  }

  // After initialization compile the shader to desired language.
  shaderc_spvc_status CompileShader(CompilationResult* result) {
    return shaderc_spvc_compile_shader(context_.get(), result->result_.get());
  }

  // Set spirv_cross decoration (added for HLSL support in Dawn)
  // Given an id, decoration and argument, the decoration flag on the id is set,
  // assuming id is valid.
  shaderc_spvc_status SetDecoration(uint32_t id,
                                    shaderc_spvc_decoration decoration,
                                    uint32_t argument) {
    return shaderc_spvc_set_decoration(context_.get(), id, decoration, argument);
  }

  // Get spirv_cross decoration (added for GLSL API support in Dawn).
  // Given an id and a decoration, result is sent out through |argument|
  // if |id| does not exist, returns an error.
  shaderc_spvc_status GetDecoration(uint32_t id,
                                    shaderc_spvc_decoration decoration,
                                    uint32_t* argument) {
    return shaderc_spvc_get_decoration(context_.get(), id, decoration, argument);
  }

  // Unset spirv_cross decoration (added for GLSL API support in Dawn).
  // Given an id and a decoration. Assuming id is valid.
  shaderc_spvc_status UnsetDecoration(uint32_t id,
                                      shaderc_spvc_decoration decoration) {
    return shaderc_spvc_unset_decoration(context_.get(), id, decoration);
  }

  // spirv-cross comment:
  // Analyzes all separate image and samplers used from the currently selected
  // entry point, and re-routes them all to a combined image sampler instead.
  // (added for GLSL API support in Dawn)
  shaderc_spvc_status BuildCombinedImageSamplers(void) {
    return shaderc_spvc_build_combined_image_samplers(context_.get());
  }

  // After call to BuildCombinedImageSamplers, fetch the ids associated with the
  // combined image samplers.
  shaderc_spvc_status GetCombinedImageSamplers(
      std::vector<shaderc_spvc_combined_image_sampler>* samplers) {
    size_t count;
    shaderc_spvc_status status = shaderc_spvc_get_combined_image_samplers(
        context_.get(), nullptr, &count);
    if (status != shaderc_spvc_status_success) return status;
    samplers->resize(count);
    return shaderc_spvc_get_combined_image_samplers(context_.get(),
                                                    samplers->data(), &count);
  }

  // set |name| on a given |id| (added for GLSL support in Dawn).
  // Assuming id is valid.
  shaderc_spvc_status SetName(uint32_t id, const std::string& name) {
    return shaderc_spvc_set_name(context_.get(), id, name.c_str());
  }

  // Adds a binding to indicate the MSL buffer, texture or sampler index to use
  // for a particular SPIR-V description set and binding.
  shaderc_spvc_status AddMSLResourceBinding(
      const shaderc_spvc_msl_resource_binding binding) {
    return shaderc_spvc_add_msl_resource_binding(context_.get(), binding);
  }

  // Gets workgroup size for an entry point defined by a given execution model
  // and function name.
  shaderc_spvc_status GetWorkgroupSize(
      const std::string& function_name,
      shaderc_spvc_execution_model execution_model,
      shaderc_spvc_workgroup_size* workgroup_size) {
    return shaderc_spvc_get_workgroup_size(
        context_.get(), function_name.c_str(), execution_model, workgroup_size);
  }

  // Gets whether or not the shader needes a buffer of buffer sizes.
  shaderc_spvc_status NeedsBufferSizeBuffer(bool* b) {
    return shaderc_spvc_needs_buffer_size_buffer(context_.get(), b);
  }

  // Gets the execution model for the shader.
  shaderc_spvc_status GetExecutionModel(shaderc_spvc_execution_model* model) {
    return shaderc_spvc_get_execution_model(context_.get(), model);
  }

  // Gets the number of push constant buffers used by the shader.
  shaderc_spvc_status GetPushConstantBufferCount(size_t* count) {
    return shaderc_spvc_get_push_constant_buffer_count(context_.get(), count);
  }

  // Gets all of the binding info for a given shader resource.
  shaderc_spvc_status GetBindingInfo(
      shaderc_spvc_shader_resource resource,
      shaderc_spvc_binding_type binding_type,
      std::vector<shaderc_spvc_binding_info>* bindings) {
    if (!bindings) {
      return shaderc_spvc_status_invalid_out_param;
    }

    size_t binding_count;
    shaderc_spvc_status status = shaderc_spvc_get_binding_info(
        context_.get(), resource, binding_type, nullptr, &binding_count);
    if (status != shaderc_spvc_status_success) {
      return status;
    }

    bindings->resize(binding_count);
    return shaderc_spvc_get_binding_info(context_.get(), resource, binding_type,
                                         bindings->data(), &binding_count);
  }

  // Gets the Location decoration information for the stage inputs.
  shaderc_spvc_status GetInputStageLocationInfo(
      std::vector<shaderc_spvc_resource_location_info>* locations) {
    if (!locations) {
      return shaderc_spvc_status_invalid_out_param;
    }

    size_t location_count;
    shaderc_spvc_status status = shaderc_spvc_get_input_stage_location_info(
        context_.get(), nullptr, &location_count);
    if (status != shaderc_spvc_status_success) {
      return status;
    }

    locations->resize(location_count);
    return shaderc_spvc_get_input_stage_location_info(
        context_.get(), locations->data(), &location_count);
  }

  // Gets the Location decoration information for the stage output.
  shaderc_spvc_status GetOutputStageLocationInfo(
      std::vector<shaderc_spvc_resource_location_info>* locations) {
    if (!locations) {
      return shaderc_spvc_status_invalid_out_param;
    }

    size_t location_count;
    shaderc_spvc_status status = shaderc_spvc_get_output_stage_location_info(
        context_.get(), nullptr, &location_count);
    if (status != shaderc_spvc_status_success) {
      return status;
    }

    locations->resize(location_count);
    return shaderc_spvc_get_output_stage_location_info(
        context_.get(), locations->data(), &location_count);
  }

  // Gets the type information for the stage output.
  shaderc_spvc_status GetOutputStageTypeInfo(
      std::vector<shaderc_spvc_resource_type_info>* types) {
    if (!types) {
      return shaderc_spvc_status_invalid_out_param;
    }

    size_t type_count;
    shaderc_spvc_status status = shaderc_spvc_get_output_stage_type_info(
        context_.get(), nullptr, &type_count);
    if (status != shaderc_spvc_status_success) {
      return status;
    }

    types->resize(type_count);
    return shaderc_spvc_get_output_stage_type_info(context_.get(),
                                                   types->data(), &type_count);
  }

 private:
  Context(const Context&) = delete;
  Context& operator=(const Context& other) = delete;

  std::unique_ptr<shaderc_spvc_context, void (*)(shaderc_spvc_context_t)>
      context_;
};

}  // namespace shaderc_spvc

#endif  // SHADERC_SPVC_HPP_
