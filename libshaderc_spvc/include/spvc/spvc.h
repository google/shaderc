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

#ifndef SHADERC_SPVC_H_
#define SHADERC_SPVC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "shaderc/env.h"
#include "shaderc/status.h"
#include "shaderc/visibility.h"

// SPIR-V decorations supported by spvc. This is not an exhaustive list of all
// of the values in the spec, but more can be added if needed.
typedef enum {
  shaderc_spvc_decoration_specid,
  shaderc_spvc_decoration_block,
  shaderc_spvc_decoration_rowmajor,
  shaderc_spvc_decoration_colmajor,
  shaderc_spvc_decoration_arraystride,
  shaderc_spvc_decoration_matrixstride,
  shaderc_spvc_decoration_builtin,
  shaderc_spvc_decoration_noperspective,
  shaderc_spvc_decoration_flat,
  shaderc_spvc_decoration_centroid,
  shaderc_spvc_decoration_restrict,
  shaderc_spvc_decoration_aliased,
  shaderc_spvc_decoration_nonwritable,
  shaderc_spvc_decoration_nonreadable,
  shaderc_spvc_decoration_uniform,
  shaderc_spvc_decoration_location,
  shaderc_spvc_decoration_component,
  shaderc_spvc_decoration_index,
  shaderc_spvc_decoration_binding,
  shaderc_spvc_decoration_descriptorset,
  shaderc_spvc_decoration_offset,
  shaderc_spvc_decoration_nocontraction,
} shaderc_spvc_decoration;

// Backwards compatiblity enum for Dawn. This will be removed once Dawn no
// longer depends on it.
#define SHADERC_SPVC_DECORATION_BINDING shaderc_spvc_decoration_binding

typedef enum {
  shaderc_spvc_msl_platform_ios,
  shaderc_spvc_msl_platform_macos,
} shaderc_spvc_msl_platform;

// Return code for spvc API calls. shaderc_spvc_status_success indicates success
// completion of the operation, all others indicate some sort of failure.
typedef enum {
  shaderc_spvc_status_success,
  shaderc_spvc_status_compilation_error,
  shaderc_spvc_status_internal_error,
  shaderc_spvc_status_validation_error,
  shaderc_spvc_status_transformation_error,
  shaderc_spvc_status_configuration_error,
  shaderc_spvc_status_uninitialized_compiler_error,
  shaderc_spvc_status_missing_context_error,
  shaderc_spvc_status_invalid_out_param,
} shaderc_spvc_status;

typedef enum {
  shaderc_spvc_execution_model_vertex,
  shaderc_spvc_execution_model_fragment,
  shaderc_spvc_execution_model_glcompute,
  shaderc_spvc_execution_model_invalid,
} shaderc_spvc_execution_model;

typedef enum {
  shaderc_spvc_binding_type_uniform_buffer = 0x00000000,
  shaderc_spvc_binding_type_storage_buffer = 0x00000001,
  shaderc_spvc_binding_type_readonly_storage_buffer = 0x00000002,
  shaderc_spvc_binding_type_sampler = 0x00000003,
  shaderc_spvc_binding_type_sampled_texture = 0x00000004,
  shaderc_spvc_binding_type_storage_texture = 0x00000005,
} shaderc_spvc_binding_type;

typedef enum {
  shaderc_spvc_texture_view_dimension_undefined = 0x00000000,
  shaderc_spvc_texture_view_dimension_e1D = 0x00000001,
  shaderc_spvc_texture_view_dimension_e2D = 0x00000002,
  shaderc_spvc_texture_view_dimension_e2D_array = 0x00000003,
  shaderc_spvc_texture_view_dimension_cube = 0x00000004,
  shaderc_spvc_texture_view_dimension_cube_array = 0x00000005,
  shaderc_spvc_texture_view_dimension_e3D = 0x00000006,
} shaderc_spvc_texture_view_dimension;

typedef enum {
  shaderc_spvc_texture_format_type_float,
  shaderc_spvc_texture_format_type_sint,
  shaderc_spvc_texture_format_type_uint,
  shaderc_spvc_texture_format_type_other,
} shaderc_spvc_texture_format_type;

typedef enum {
  shaderc_spvc_shader_resource_uniform_buffers,
  shaderc_spvc_shader_resource_separate_images,
  shaderc_spvc_shader_resource_separate_samplers,
  shaderc_spvc_shader_resource_storage_buffers,
} shaderc_spvc_shader_resource;

// An opaque handle to an object that manages all compiler state.
typedef struct shaderc_spvc_context* shaderc_spvc_context_t;

typedef struct {
  uint32_t combined_id;
  uint32_t image_id;
  uint32_t sampler_id;
} shaderc_spvc_combined_image_sampler;

typedef struct {
  shaderc_spvc_execution_model stage;
  uint32_t desc_set;
  uint32_t binding;
  uint32_t msl_buffer;
  uint32_t msl_texture;
  uint32_t msl_sampler;
} shaderc_spvc_msl_resource_binding;

typedef struct {
  uint32_t x;
  uint32_t y;
  uint32_t z;
  uint32_t constant;
} shaderc_spvc_workgroup_size;

typedef struct {
  uint32_t set;
  uint32_t binding;
  uint32_t id;
  uint32_t base_type_id;
  shaderc_spvc_binding_type binding_type;
  shaderc_spvc_texture_view_dimension texture_dimension;
  shaderc_spvc_texture_format_type texture_component_type;
  bool multisampled;
} shaderc_spvc_binding_info;

typedef struct {
  uint32_t id;
  bool has_location;
  uint32_t location;
} shaderc_spvc_resource_location_info;

typedef struct {
  uint32_t location;
  shaderc_spvc_texture_format_type type;
} shaderc_spvc_resource_type_info;

// Create a spvc state handle.  A return of NULL indicates that there was an
// error. Any function operating on a *_context_t must offer the basic
// thread-safety guarantee.
// [http://herbsutter.com/2014/01/13/gotw-95-solution-thread-safety-and-synchronization/]
// That is: concurrent invocation of these functions on DIFFERENT objects needs
// no synchronization; concurrent invocation of these functions on the SAME
// object requires synchronization IF AND ONLY IF some of them take a non-const
// argument.
SHADERC_EXPORT shaderc_spvc_context_t shaderc_spvc_context_create(void);

// Release resources.  After this the handle cannot be used.
SHADERC_EXPORT void shaderc_spvc_context_destroy(
    shaderc_spvc_context_t context);

// Get validation/compilation error or informational messages.
SHADERC_EXPORT const char* shaderc_spvc_context_get_messages(
    const shaderc_spvc_context_t context);

// EXPERIMENTAL
// Get spirv_cross compiler reference, does NOT transfer ownership.
// Return type is actually spirv_cross::Compiler*, but cannot have that in the
// C API.
// This is being exposed temporarily to ease integration of spvc into Dawn, but
// this is will be removed in the future without warning.
SHADERC_EXPORT void* shaderc_spvc_context_get_compiler(
    const shaderc_spvc_context_t context);

// If true, use spvc built in parser to generate IR for spirv-cross, otherwise
// use spirv-cross's implementation.
SHADERC_EXPORT void shaderc_spvc_context_set_use_spvc_parser(
    shaderc_spvc_context_t context, bool b);

// An opaque handle to an object that manages options to a single compilation
// result.
typedef struct shaderc_spvc_compile_options* shaderc_spvc_compile_options_t;

// Creates default compiler options.
// A return of NULL indicates that there was an error initializing the options.
// Any function operating on shaderc_spvc_compile_options_t must offer the
// basic thread-safety guarantee.
SHADERC_EXPORT shaderc_spvc_compile_options_t
shaderc_spvc_compile_options_create(void);

// Returns a copy of the given options.
// If NULL is passed as the parameter the call is the same as
// shaderc_spvc_compile_options_init.
SHADERC_EXPORT shaderc_spvc_compile_options_t
shaderc_spvc_compile_options_clone(
    const shaderc_spvc_compile_options_t options);

// Destroys the compilation options. It is invalid to use the given
// option object in any future calls. It is safe to pass
// NULL to this function, and doing such will have no effect.
SHADERC_EXPORT void shaderc_spvc_compile_options_destroy(
    shaderc_spvc_compile_options_t options);

// Sets the entry point.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_entry_point(
    shaderc_spvc_compile_options_t options, const char* entry_point);

// If true, unused variables will not appear in the output.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_remove_unused_variables(
    shaderc_spvc_compile_options_t options, bool b);

// If true, enable robust buffer access pass in the spirv-opt, meaning:
// Inject code to clamp indexed accesses to buffers and internal
// arrays, providing guarantees satisfying Vulkan's robustBufferAccess rules.
// This is useful when an implementation does not support robust-buffer access
// as a driver option.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_robust_buffer_access_pass(
    shaderc_spvc_compile_options_t options, bool b);

SHADERC_EXPORT void shaderc_spvc_compile_options_set_emit_line_directives(
    shaderc_spvc_compile_options_t options, bool b);
// Sets the source shader environment, affecting which warnings or errors will
// be issued during validation.
// Default value for environment is Vulkan 1.0.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_source_env(
    shaderc_spvc_compile_options_t options, shaderc_target_env env,
    shaderc_env_version version);

// Sets the target shader environment, if this is different from the source
// environment, then a transform between the environments will be performed if
// possible. Currently only WebGPU <-> Vulkan 1.1 are defined.
// Default value for environment is Vulkan 1.0.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_target_env(
    shaderc_spvc_compile_options_t options, shaderc_target_env env,
    shaderc_env_version version);

// If true, Vulkan GLSL features are used instead of GL-compatible features.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_vulkan_semantics(
    shaderc_spvc_compile_options_t options, bool b);

// If true, gl_PerVertex is explicitly redeclared in vertex, geometry and
// tessellation shaders. The members of gl_PerVertex is determined by which
// built-ins are declared by the shader.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_separate_shader_objects(
    shaderc_spvc_compile_options_t options, bool b);

// Flatten uniform or push constant variable into (i|u)vec4 array.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_flatten_ubo(
    shaderc_spvc_compile_options_t options, bool b);

// Set GLSL language version.  Default is 450 (i.e. 4.5).
SHADERC_EXPORT void shaderc_spvc_compile_options_set_glsl_language_version(
    shaderc_spvc_compile_options_t options, uint32_t version);

// If true, flatten multidimensional arrays, e.g. foo[a][b][c] -> foo[a*b*c].
// Default is false.
SHADERC_EXPORT void
shaderc_spvc_compile_options_set_flatten_multidimensional_arrays(
    shaderc_spvc_compile_options_t options, bool b);

// Force interpretion as ES, or not.  Default is to detect from source.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_es(
    shaderc_spvc_compile_options_t options, bool b);

// If true, emit push constants as uniform buffer objects.  Default is false.
SHADERC_EXPORT void
shaderc_spvc_compile_options_set_glsl_emit_push_constant_as_ubo(
    shaderc_spvc_compile_options_t options, bool b);

// Set MSL language version.  Default is 10200 (i.e. 1.2).
SHADERC_EXPORT void shaderc_spvc_compile_options_set_msl_language_version(
    shaderc_spvc_compile_options_t options, uint32_t version);

// If true, swizzle MSL texture samples.  Default is false.
SHADERC_EXPORT void
shaderc_spvc_compile_options_set_msl_swizzle_texture_samples(
    shaderc_spvc_compile_options_t options, bool b);

// Choose MSL platform.  Default is MacOS.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_msl_platform(
    shaderc_spvc_compile_options_t options, shaderc_spvc_msl_platform platform);

// If true, pad MSL fragment output.  Default is false.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_msl_pad_fragment_output(
    shaderc_spvc_compile_options_t options, bool b);

// If true, capture MSL output to buffer.  Default is false.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_msl_capture(
    shaderc_spvc_compile_options_t options, bool b);

// If true, flip the Y-coord of the built-in "TessCoord."  Default is top left.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_msl_domain_lower_left(
    shaderc_spvc_compile_options_t options, bool b);

// Enable use of MSL 2.0 indirect argument buffers.  Default is not to use them.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_msl_argument_buffers(
    shaderc_spvc_compile_options_t options, bool b);

// When using MSL argument buffers, force "classic" MSL 1.0 binding for the
// given descriptor sets. This corresponds to VK_KHR_push_descriptor in Vulkan.
SHADERC_EXPORT void
shaderc_spvc_compile_options_set_msl_discrete_descriptor_sets(
    shaderc_spvc_compile_options_t options, const uint32_t* descriptors,
    size_t num_descriptors);

// Set whether or not PointSize builtin is used for MSL shaders
SHADERC_EXPORT void
shaderc_spvc_compile_options_set_msl_enable_point_size_builtin(
    shaderc_spvc_compile_options_t options, bool b);

// Set the index in the buffer size in the buffer for MSL
SHADERC_EXPORT void
shaderc_spvc_compile_options_set_msl_buffer_size_buffer_index(
    shaderc_spvc_compile_options_t options, uint32_t index);

// Set HLSL shader model.  Default is 30.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_hlsl_shader_model(
    shaderc_spvc_compile_options_t options, uint32_t model);

// If true, ignore PointSize.  Default is false.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_hlsl_point_size_compat(
    shaderc_spvc_compile_options_t options, bool b);

// If true, ignore PointCoord.  Default is false.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_hlsl_point_coord_compat(
    shaderc_spvc_compile_options_t options, bool b);

// If true (default is false):
//   GLSL: map depth from Vulkan/D3D style to GL style, i.e. [ 0,w] -> [-w,w]
//   MSL : map depth from GL style to Vulkan/D3D style, i.e. [-w,w] -> [ 0,w]
//   HLSL: map depth from GL style to Vulkan/D3D style, i.e. [-w,w] -> [ 0,w]
SHADERC_EXPORT void shaderc_spvc_compile_options_set_fixup_clipspace(
    shaderc_spvc_compile_options_t options, bool b);

// If true invert gl_Position.y or equivalent.  Default is false.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_flip_vert_y(
    shaderc_spvc_compile_options_t options, bool b);

// Set if validation should be performed. Default is true.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_validate(
    shaderc_spvc_compile_options_t options, bool b);

// Set if optimization should be performed. Default is true.
SHADERC_EXPORT void shaderc_spvc_compile_options_set_optimize(
    shaderc_spvc_compile_options_t options, bool b);

// Fill options with given data.  Return amount of data used, or zero
// if not enough data was given.
SHADERC_EXPORT size_t shaderc_spvc_compile_options_set_for_fuzzing(
    shaderc_spvc_compile_options_t options, const uint8_t* data, size_t size);

// An opaque handle to the results of a call to any
// shaderc_spvc_*_compile_*() function.
typedef struct shaderc_spvc_compilation_result*
    shaderc_spvc_compilation_result_t;

// Takes SPIR-V as a sequence of 32-bit words, validates it, then creates the
// internal compiler for translating to GLSL and performing reflection.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_initialize_for_glsl(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options);

// Takes SPIR-V as a sequence of 32-bit words, validates it, then creates the
// internal compiler for translating to HLSL and performing reflection.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_initialize_for_hlsl(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options);

// Takes SPIR-V as a sequence of 32-bit words, validates it, then creates the
// internal compiler for translating to MSL and performing reflection.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_initialize_for_msl(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options);

// Takes SPIR-V as a sequence of 32-bit words, validates it, then creates the
// internal compiler for translating to Vulkan and performing reflection.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_initialize_for_vulkan(
    const shaderc_spvc_context_t context, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options);

// Given an initialized compiler, generates a shader of the appropriate
// language.
SHADERC_EXPORT shaderc_spvc_status
shaderc_spvc_compile_shader(const shaderc_spvc_context_t context,
                            shaderc_spvc_compilation_result_t result);

// Get spirv_cross decoration (added for GLSL API support in Dawn)
// Given an id and a decoration, result is sent out through |argument|
// if |id| does not exist, returns an error.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_get_decoration(
    const shaderc_spvc_context_t context, uint32_t id,
    shaderc_spvc_decoration decoration, uint32_t* value);

// Unset spirv_cross decoration (added for GLSL API support in Dawn)
// Given an id and a decoration. Assuming |id| is valid.
SHADERC_EXPORT shaderc_spvc_status
shaderc_spvc_unset_decoration(const shaderc_spvc_context_t context, uint32_t id,
                              shaderc_spvc_decoration decoration);

// Set |name| on a given |id| (added for GLSL API support in Dawn)
// Assuming |id| is valid.
SHADERC_EXPORT void shaderc_spvc_set_name(const shaderc_spvc_context_t context,
                                          uint32_t id, const char* name);

// spirv-cross comment:
// Analyzes all separate image and samplers used from the currently selected
// entry point, and re-routes them all to a combined image sampler instead.
// (added for GLSL API support in Dawn)
SHADERC_EXPORT void shaderc_spvc_build_combined_image_samplers(
    const shaderc_spvc_context_t context);

// Returns the combined image samplers.

// If |samples| is NULL, then num_samplers is set, and no data is copied.
// The caller is responsible for |samplers| being large enough to
// contain all of the data.
SHADERC_EXPORT void shaderc_spvc_get_combined_image_samplers(
    const shaderc_spvc_context_t context,
    shaderc_spvc_combined_image_sampler* samplers, size_t* num_samplers);

// Set spirv_cross decoration (added for HLSL support in Dawn)
// Given an id, decoration and argument, the decoration flag on the id is set
// Assuming id is valid.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_set_decoration(
    const shaderc_spvc_context_t context, uint32_t id,
    shaderc_spvc_decoration decoration, uint32_t argument);

// Adds a binding to indicate the MSL buffer, texture or sampler index to use
// for a particular SPIR-V description set and binding.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_add_msl_resource_binding(
    const shaderc_spvc_context_t context,
    const shaderc_spvc_msl_resource_binding binding);

// Gets workgroup size for an entry point defined by a given execution model and
// function name.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_get_workgroup_size(
    const shaderc_spvc_context_t context, const char* function_name,
    shaderc_spvc_execution_model execution_model,
    shaderc_spvc_workgroup_size* workgroup_size);

// Gets whether or not the shader needes a buffer of buffer sizes.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_needs_buffer_size_buffer(
    const shaderc_spvc_context_t context, bool* b);

// Gets the execution model for the shader parsed by the compiler.
SHADERC_EXPORT shaderc_spvc_status
shaderc_spvc_get_execution_model(const shaderc_spvc_context_t context,
                                 shaderc_spvc_execution_model* execution_model);

// Gets the number of push constants buffers used by the shader.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_get_push_constant_buffer_count(
    const shaderc_spvc_context_t context, size_t* count);

// Fetches all of the binding info for a given shader resource.
// If |bindings| is null, then |binding_count| is populated with the number of
// entries that would have been written out.
// The caller is responsible for ensuring that |bindings| has enough space
// allocated to contain all of the entries.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_get_binding_info(
    const shaderc_spvc_context_t context, shaderc_spvc_shader_resource resource,
    shaderc_spvc_binding_type binding_type, shaderc_spvc_binding_info* bindings,
    size_t* binding_count);

// Fetches the Location decoration information for the stage inputs.
// If |locations| is null, then |location_count| is populated with the number of
// entries that would have been written out.
// The caller is responsible for ensuring that |locations| has enough space
// allocated to contain all of the entries.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_get_input_stage_location_info(
    const shaderc_spvc_context_t context,
    shaderc_spvc_resource_location_info* locations, size_t* location_count);

// Fetches the Location decoration information for the stage outputs.
// If |locations| is null, then |location_count| is populated with the number of
// entries that would have been written out.
// The caller is responsible for ensuring that |locations| has enough space
// allocated to contain all of the entries.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_get_output_stage_location_info(
    const shaderc_spvc_context_t context,
    shaderc_spvc_resource_location_info* locations, size_t* location_count);

// Fetches the type information for the stage outputs.
// If |types| is null, then |type_count| is populated with the number of
// entries that would have been written out.
// The caller is responsible for ensuring that |types| has enough space
// allocated to contain all of the entries.
SHADERC_EXPORT shaderc_spvc_status shaderc_spvc_get_output_stage_type_info(
    const shaderc_spvc_context_t context,
    shaderc_spvc_resource_type_info* types, size_t* type_count);

// The following functions, operating on shaderc_spvc_compilation_result_t
// objects, offer only the basic thread-safety guarantee.

// Creates an instant of compiliation result data structure.
// A return of NULL indicates that there was an error creating the structure.
// Any function operating on shaderc_spvc_compilation_result_t must offer the
// basic thread-safety guarantee.
SHADERC_EXPORT shaderc_spvc_compilation_result_t
shaderc_spvc_result_create(void);

// Destroys the resources held by the result object. It is invalid to use the
// result object for any further operations.
SHADERC_EXPORT void shaderc_spvc_result_destroy(
    shaderc_spvc_compilation_result_t result);

// Get validation/compilation result as a string. This is only supported
// compiling to GLSL, HSL, and MSL.
SHADERC_EXPORT const char* shaderc_spvc_result_get_string_output(
    const shaderc_spvc_compilation_result_t result);

// Get validation/compilation result as a binary buffer. This is only supported
// compiling to Vulkan.
SHADERC_EXPORT const uint32_t* shaderc_spvc_result_get_binary_output(
    const shaderc_spvc_compilation_result_t result);

// Get length of validation/compilation result as a binary buffer. This is only
// supported compiling to Vulkan.
SHADERC_EXPORT uint32_t shaderc_spvc_result_get_binary_length(
    const shaderc_spvc_compilation_result_t result);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // SHADERC_SPVC_H_
