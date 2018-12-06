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

#ifndef SHADERC_SPVC_SPVC_H_
#define SHADERC_SPVC_SPVC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//XXX share with libshaderc?
// SHADERC_EXPORT tags symbol that will be exposed by the shared library.
#if defined(SHADERC_SHAREDLIB)
    #if defined(_WIN32)
        #if defined(SHADERC_IMPLEMENTATION)
            #define SHADERC_EXPORT __declspec(dllexport)
        #else
            #define SHADERC_EXPORT __declspec(dllimport)
        #endif
    #else
        #if defined(SHADERC_IMPLEMENTATION)
            #define SHADERC_EXPORT __attribute__((visibility("default")))
        #else
            #define SHADERC_EXPORT
        #endif
    #endif
#else
    #define SHADERC_EXPORT
#endif

//XXX share with libshaderc?
// Indicate the status of a compilation.
typedef enum {
  spvc_compilation_status_success = 0,
  spvc_compilation_status_invalid_stage,  // error stage deduction
  spvc_compilation_status_compilation_error,
  spvc_compilation_status_internal_error,  // unexpected failure
  spvc_compilation_status_null_result_object,
  spvc_compilation_status_invalid_assembly,
} spvc_compilation_status;

// An opaque handle to an object that manages all compiler state.
typedef struct spvc_compiler* spvc_compiler_t;

// Returns a spvc_compiler_t that can be used to compile modules.
// A return of NULL indicates that there was an error initializing the compiler.
// Any function operating on spvc_compiler_t must offer the basic
// thread-safety guarantee.
// [http://herbsutter.com/2014/01/13/gotw-95-solution-thread-safety-and-synchronization/]
// That is: concurrent invocation of these functions on DIFFERENT objects needs
// no synchronization; concurrent invocation of these functions on the SAME
// object requires synchronization IF AND ONLY IF some of them take a non-const
// argument.
SHADERC_EXPORT spvc_compiler_t spvc_compiler_initialize(void);

// Releases the resources held by the spvc_compiler_t.
// After this call it is invalid to make any future calls to functions
// involving this spvc_compiler_t.
SHADERC_EXPORT void spvc_compiler_release(spvc_compiler_t);

// An opaque handle to an object that manages options to a single compilation
// result.
typedef struct spvc_compile_options* spvc_compile_options_t;

// Returns a default-initialized spvc_compile_options_t that can be used
// to modify the functionality of a compiled module.
// A return of NULL indicates that there was an error initializing the options.
// Any function operating on spvc_compile_options_t must offer the
// basic thread-safety guarantee.
SHADERC_EXPORT spvc_compile_options_t
    spvc_compile_options_initialize(void);

// Returns a copy of the given spvc_compile_options_t.
// If NULL is passed as the parameter the call is the same as
// spvc_compile_options_init.
SHADERC_EXPORT spvc_compile_options_t spvc_compile_options_clone(
    const spvc_compile_options_t options);

// Releases the compilation options. It is invalid to use the given
// spvc_compile_options_t object in any future calls. It is safe to pass
// NULL to this function, and doing such will have no effect.
SHADERC_EXPORT void spvc_compile_options_release(
    spvc_compile_options_t options);

// An opaque handle to the results of a call to any spvc_compile_into_*()
// function.
typedef struct spvc_compilation_result* spvc_compilation_result_t;

// Takes SPIR-V as a sequence of 32-bit words and compiles to GLSL.
SHADERC_EXPORT spvc_compilation_result_t spvc_compile_into_glsl(
    const spvc_compiler_t compiler, const uint32_t* source,
    size_t source_len, const spvc_compile_options_t additional_options);

// The following functions, operating on spvc_compilation_result_t objects,
// offer only the basic thread-safety guarantee.

// Releases the resources held by the result object. It is invalid to use the
// result object for any further operations.
SHADERC_EXPORT void spvc_result_release(spvc_compilation_result_t result);

// Returns the number of bytes of the compilation output data in a result
// object.
SHADERC_EXPORT size_t spvc_result_get_length(const spvc_compilation_result_t result);

// Returns the number of warnings generated during the compilation.
SHADERC_EXPORT size_t spvc_result_get_num_warnings(
    const spvc_compilation_result_t result);

// Returns the number of errors generated during the compilation.
SHADERC_EXPORT size_t spvc_result_get_num_errors(const spvc_compilation_result_t result);

// Returns the compilation status, indicating whether the compilation succeeded,
// or failed due to some reasons, like invalid shader stage or compilation
// errors.
SHADERC_EXPORT spvc_compilation_status spvc_result_get_compilation_status(
    const spvc_compilation_result_t);

// Returns a pointer to the start of the compilation output data bytes, either
// SPIR-V binary or char string. When the source string is compiled into SPIR-V
// binary, this is guaranteed to be castable to a uint32_t*. If the result
// contains assembly text or preprocessed source text, the pointer will point to
// the resulting array of characters.
SHADERC_EXPORT const char* spvc_result_get_bytes(const spvc_compilation_result_t result);

// Returns a null-terminated string that contains any error messages generated
// during the compilation.
SHADERC_EXPORT const char* spvc_result_get_error_message(
    const spvc_compilation_result_t result);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // SHADERC_SPVC_SPVC_H_
