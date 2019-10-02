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

typedef enum {
  shaderc_spvc_msl_platform_ios,
  shaderc_spvc_msl_platform_macos,
} shaderc_spvc_msl_platform;

// An opaque handle to an object that manages all compiler state.
typedef struct shaderc_spvc_compiler* shaderc_spvc_compiler_t;

// Create a compiler.  A return of NULL indicates that there was an error.
// Any function operating on a *_compiler_t must offer the basic
// thread-safety guarantee.
// [http://herbsutter.com/2014/01/13/gotw-95-solution-thread-safety-and-synchronization/]
// That is: concurrent invocation of these functions on DIFFERENT objects needs
// no synchronization; concurrent invocation of these functions on the SAME
// object requires synchronization IF AND ONLY IF some of them take a non-const
// argument.
SHADERC_EXPORT shaderc_spvc_compiler_t shaderc_spvc_compiler_initialize(void);

// Release resources.  After this the handle cannot be used.
SHADERC_EXPORT void shaderc_spvc_compiler_release(shaderc_spvc_compiler_t);

// An opaque handle to an object that manages options to a single compilation
// result.
typedef struct shaderc_spvc_compile_options* shaderc_spvc_compile_options_t;

// Returns default compiler options.
// A return of NULL indicates that there was an error initializing the options.
// Any function operating on shaderc_spvc_compile_options_t must offer the
// basic thread-safety guarantee.
SHADERC_EXPORT shaderc_spvc_compile_options_t
shaderc_spvc_compile_options_initialize(void);

// Returns a copy of the given options.
// If NULL is passed as the parameter the call is the same as
// shaderc_spvc_compile_options_init.
SHADERC_EXPORT shaderc_spvc_compile_options_t
shaderc_spvc_compile_options_clone(
    const shaderc_spvc_compile_options_t options);

// Releases the compilation options. It is invalid to use the given
// option object in any future calls. It is safe to pass
// NULL to this function, and doing such will have no effect.
SHADERC_EXPORT void shaderc_spvc_compile_options_release(
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
// shaderc_spvc_compile_into_*() function.
typedef struct shaderc_spvc_compilation_result*
    shaderc_spvc_compilation_result_t;

// Takes SPIR-V as a sequence of 32-bit words, validates it, then compiles to
// GLSL.
SHADERC_EXPORT shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_glsl(
    const shaderc_spvc_compiler_t compiler, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options);

// Takes SPIR-V as a sequence of 32-bit words, validates it, then compiles to
// HLSL.
SHADERC_EXPORT shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_hlsl(
    const shaderc_spvc_compiler_t compiler, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options);

// Takes SPIR-V as a sequence of 32-bit words, validates it, then compiles to
// MSL.
SHADERC_EXPORT shaderc_spvc_compilation_result_t shaderc_spvc_compile_into_msl(
    const shaderc_spvc_compiler_t compiler, const uint32_t* source,
    size_t source_len, shaderc_spvc_compile_options_t options);

// Takes SPIR-V as a sequence of 32-bit words, validates it, then compiles to
// Vullkan specific SPIR-V.
SHADERC_EXPORT shaderc_spvc_compilation_result_t
shaderc_spvc_compile_into_vulkan(const shaderc_spvc_compiler_t compiler,
                                 const uint32_t* source, size_t source_len,
                                 shaderc_spvc_compile_options_t options);

// The following functions, operating on shaderc_spvc_compilation_result_t
// objects, offer only the basic thread-safety guarantee.

// Releases the resources held by the result object. It is invalid to use the
// result object for any further operations.
SHADERC_EXPORT void shaderc_spvc_result_release(
    shaderc_spvc_compilation_result_t result);

// Returns the compilation status, indicating whether the compilation succeeded,
// or failed due to some reasons, like invalid shader stage or compilation
// errors.
SHADERC_EXPORT shaderc_compilation_status
shaderc_spvc_result_get_status(const shaderc_spvc_compilation_result_t);

// Get validation/compilation error or informational messages.
SHADERC_EXPORT const char* shaderc_spvc_result_get_messages(
    const shaderc_spvc_compilation_result_t result);

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
