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
  explicit CompilationResult(shaderc_spvc_compilation_result_t result)
      : result_(result) {}
  CompilationResult() : result_(nullptr) {}
  ~CompilationResult() { shaderc_spvc_result_release(result_); }

  CompilationResult(CompilationResult&& other) : result_(nullptr) {
    *this = std::move(other);
  }

  CompilationResult& operator=(CompilationResult&& other) {
    if (result_) {
      shaderc_spvc_result_release(result_);
    }
    result_ = other.result_;
    other.result_ = nullptr;
    return *this;
  }

  // Returns the compilation status, indicating whether the compilation
  // succeeded, or failed due to some reasons, like invalid shader stage or
  // compilation errors.
  shaderc_compilation_status GetCompilationStatus() const {
    if (!result_) {
      return shaderc_compilation_status_null_result_object;
    }
    return shaderc_spvc_result_get_status(result_);
  }

  const std::string GetStringOutput() const {
    return shaderc_spvc_result_get_string_output(result_);
  }

  const std::vector<uint32_t> GetBinaryOutput() const {
    const uint32_t* binary_output =
        shaderc_spvc_result_get_binary_output(result_);
    uint32_t binary_length = shaderc_spvc_result_get_binary_length(result_);
    if (!binary_output || !binary_length) return {};

    return std::vector<uint32_t>(binary_output, binary_output + binary_length);
  }

  const std::string GetMessages() const {
    return shaderc_spvc_result_get_messages(result_);
  }

 private:
  CompilationResult(const CompilationResult& other) = delete;
  CompilationResult& operator=(const CompilationResult& other) = delete;

  shaderc_spvc_compilation_result_t result_;
};

// Contains any options that can have default values for a compilation.
class CompileOptions {
 public:
  CompileOptions() { options_ = shaderc_spvc_compile_options_initialize(); }
  ~CompileOptions() { shaderc_spvc_compile_options_release(options_); }
  CompileOptions(const CompileOptions& other) {
    options_ = shaderc_spvc_compile_options_clone(other.options_);
  }
  CompileOptions(CompileOptions&& other) {
    options_ = other.options_;
    other.options_ = nullptr;
  }

  // Set the environment for the input SPIR-V.  Default is Vulkan 1.0.
  void SetSourceEnvironment(shaderc_target_env env,
                            shaderc_env_version version) {
    shaderc_spvc_compile_options_set_source_env(options_, env, version);
  }

  // Set the target environment for the SPIR-V to be cross-compiled. If this is
  // different then the source a transformation will need to be applied.
  // Currently only Vulkan 1.1 <-> WebGPU transforms are defined. Default is
  // Vulkan 1.0.
  void SetTargetEnvironment(shaderc_target_env env,
                            shaderc_env_version version) {
    shaderc_spvc_compile_options_set_target_env(options_, env, version);
  }

  // Set the entry point.
  void SetEntryPoint(const std::string& entry_point) {
    shaderc_spvc_compile_options_set_entry_point(options_, entry_point.c_str());
  }

  // If true, unused variables will not appear in the output.
  void SetRemoveUnusedVariables(bool b) {
    shaderc_spvc_compile_options_set_remove_unused_variables(options_, b);
  }

  // If true, enable robust buffer access pass in the spirv-opt, meaning:
  // Inject code to clamp indexed accesses to buffers and internal
  // arrays, providing guarantees satisfying Vulkan's robustBufferAccess rules.
  // This is useful when an implementation does not support robust-buffer access
  // as a driver option.
  void SetRobustBufferAccessPass(bool b){
    shaderc_spvc_compile_options_set_robust_buffer_access_pass(options_, b);
  }
  // If true, Vulkan GLSL features are used instead of GL-compatible features.
  void SetVulkanSemantics(bool b) {
    shaderc_spvc_compile_options_set_vulkan_semantics(options_, b);
  }

  // If true, gl_PerVertex is explicitly redeclared in vertex, geometry and
  // tessellation shaders. The members of gl_PerVertex is determined by which
  // built-ins are declared by the shader.
  void SetSeparateShaderObjects(bool b) {
    shaderc_spvc_compile_options_set_separate_shader_objects(options_, b);
  }

  // Flatten uniform or push constant variable into (i|u)vec4 array.
  void SetFlattenUbo(bool b) {
    shaderc_spvc_compile_options_set_flatten_ubo(options_, b);
  }

  // Which GLSL version should be produced.  Default is 450 (i.e. 4.5).
  void SetGLSLLanguageVersion(uint32_t version) {
    shaderc_spvc_compile_options_set_glsl_language_version(options_, version);
  }

  // If true, flatten multidimensional arrays, e.g. foo[a][b][c] -> foo[a*b*c].
  // Default is false.
  void SetFlattenMultidimensionalArrays(bool b) {
    shaderc_spvc_compile_options_set_flatten_multidimensional_arrays(options_,
                                                                     b);
  }

  // Force interpretion as ES, or not.  Default is to detect from source.
  void SetES(bool b) { shaderc_spvc_compile_options_set_es(options_, b); }

  // If true, emit push constants as uniform buffer objects.  Default is false.
  void SetGLSLEmitPushConstantAsUBO(bool b) {
    shaderc_spvc_compile_options_set_glsl_emit_push_constant_as_ubo(options_,
                                                                    b);
  }

  // Which MSL version should be produced.  Default is 10200 (i.e. 1.2).
  void SetMSLLanguageVersion(uint32_t version) {
    shaderc_spvc_compile_options_set_msl_language_version(options_, version);
  }

  // If true, swizzle MSL texture samples.  Default is false.
  void SetMSLSwizzleTextureSamples(bool b) {
    shaderc_spvc_compile_options_set_msl_swizzle_texture_samples(options_, b);
  }

  // Choose MSL platform.  Default is MacOS.
  void SetMSLPlatform(shaderc_spvc_msl_platform platform) {
    shaderc_spvc_compile_options_set_msl_platform(options_, platform);
  }

  // If true, pad MSL fragment output.  Default is false.
  void SetMSLPadFragmentOutput(bool b) {
    shaderc_spvc_compile_options_set_msl_pad_fragment_output(options_, b);
  }

  // If true, capture MSL output to buffer.  Default is false.
  void SetMSLCapture(bool b) {
    shaderc_spvc_compile_options_set_msl_capture(options_, b);
  }

  // If true, flip the Y-coord of the built-in "TessCoord."  Default is top
  // left.
  void SetMSLDomainLowerLeft(bool b) {
    shaderc_spvc_compile_options_set_msl_domain_lower_left(options_, b);
  }

  // Enable use of MSL 2.0 indirect argument buffers.  Default is not to use
  // them.
  void SetMSLArgumentBuffers(bool b) {
    shaderc_spvc_compile_options_set_msl_argument_buffers(options_, b);
  }

  // When using MSL argument buffers, force "classic" MSL 1.0 binding for the
  // given descriptor sets. This corresponds to VK_KHR_push_descriptor in
  // Vulkan.
  void SetMSLDiscreteDescriptorSets(const std::vector<uint32_t> descriptors) {
    shaderc_spvc_compile_options_set_msl_discrete_descriptor_sets(
        options_, descriptors.data(), descriptors.size());
  }

  // Which HLSL shader model should be used.  Default is 30.
  void SetHLSLShaderModel(uint32_t model) {
    shaderc_spvc_compile_options_set_hlsl_shader_model(options_, model);
  }

  // If true, ignore PointSize.  Default is false.
  void SetHLSLPointSizeCompat(bool b) {
    shaderc_spvc_compile_options_set_hlsl_point_size_compat(options_, b);
  }

  // If true, ignore PointCoord.  Default is false.
  void SetHLSLPointCoordCompat(bool b) {
    shaderc_spvc_compile_options_set_hlsl_point_coord_compat(options_, b);
  }

  // If true (default is false):
  //   GLSL: map depth from Vulkan/D3D style to GL style, i.e. [ 0,w] -> [-w,w]
  //   MSL : map depth from GL style to Vulkan/D3D style, i.e. [-w,w] -> [ 0,w]
  //   HLSL: map depth from GL style to Vulkan/D3D style, i.e. [-w,w] -> [ 0,w]
  void SetFixupClipspace(bool b) {
    shaderc_spvc_compile_options_set_fixup_clipspace(options_, b);
  }

  // If true invert gl_Position.y or equivalent.  Default is false.
  void SetFlipVertY(bool b) {
    shaderc_spvc_compile_options_set_flip_vert_y(options_, b);
  }

  // If true validate input and intermediate source. Default is true.
  void SetValidate(bool b) {
    shaderc_spvc_compile_options_set_validate(options_, b);
  }

  // If true optimize input and intermediate source. Default is true.
  void SetOptimize(bool b) {
    shaderc_spvc_compile_options_set_optimize(options_, b);
  }

  // Fill options with given data.  Return amount of data used, or zero
  // if not enough data was given.
  size_t SetForFuzzing(const uint8_t* data, size_t size) {
    return shaderc_spvc_compile_options_set_for_fuzzing(options_, data, size);
  }

 private:
  CompileOptions& operator=(const CompileOptions& other) = delete;
  shaderc_spvc_compile_options_t options_;

  friend class Compiler;
};

// The compilation context for compiling SPIR-V.
class Compiler {
 public:
  Compiler() : compiler_(shaderc_spvc_compiler_initialize()) {}
  ~Compiler() { shaderc_spvc_compiler_release(compiler_); }

  Compiler(Compiler&& other) {
    compiler_ = other.compiler_;
    other.compiler_ = nullptr;
  }

  bool IsValid() const { return compiler_ != nullptr; }

  // Compiles the given source SPIR-V to GLSL.
  CompilationResult CompileSpvToGlsl(const uint32_t* source, size_t source_len,
                                     const CompileOptions& options) const {
    shaderc_spvc_compilation_result_t compilation_result =
        shaderc_spvc_compile_into_glsl(compiler_, source, source_len,
                                       options.options_);
    return CompilationResult(compilation_result);
  }

  // Compiles the given source SPIR-V to HLSL.
  CompilationResult CompileSpvToHlsl(const uint32_t* source, size_t source_len,
                                     const CompileOptions& options) const {
    shaderc_spvc_compilation_result_t compilation_result =
        shaderc_spvc_compile_into_hlsl(compiler_, source, source_len,
                                       options.options_);
    return CompilationResult(compilation_result);
  }

  // Compiles the given source SPIR-V to MSL.
  CompilationResult CompileSpvToMsl(const uint32_t* source, size_t source_len,
                                    const CompileOptions& options) const {
    shaderc_spvc_compilation_result_t compilation_result =
        shaderc_spvc_compile_into_msl(compiler_, source, source_len,
                                      options.options_);
    return CompilationResult(compilation_result);
  }

  // Compiles the given source SPIR-V to Vulkan SPIR-V.
  CompilationResult CompileSpvToVulkan(const uint32_t* source,
                                       size_t source_len,
                                       const CompileOptions& options) const {
    shaderc_spvc_compilation_result_t compilation_result =
        shaderc_spvc_compile_into_vulkan(compiler_, source, source_len,
                                         options.options_);
    return CompilationResult(compilation_result);
  }

 private:
  Compiler(const Compiler&) = delete;
  Compiler& operator=(const Compiler& other) = delete;

  shaderc_spvc_compiler_t compiler_;
};
}  // namespace shaderc_spvc

#endif  // SHADERC_SPVC_HPP_
