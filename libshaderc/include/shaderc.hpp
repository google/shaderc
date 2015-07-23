// Copyright 2015 The Shaderc Authors. All rights reserved.
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

#ifndef SHADERC_HPP_
#define SHADERC_HPP_

#include <string>
#include <vector>

#include "shaderc.h"

namespace shaderc {
// Contains the result of a compilation to SPIR-V.
class SpvModule {
 public:
  // Upon creation, the SpvModule takes ownership of the shaderc_spv_module_t.
  // During destruction of the SpvModule, the shaderc_spv_module_t will be
  // released.
  explicit SpvModule(shaderc_spv_module_t module) : module_(module) {}
  ~SpvModule() { shaderc_module_release(module_); }

  SpvModule(SpvModule&& other) {
    module_ = other.module_;
    other.module_ = nullptr;
  }

  // Returns true if the module was successfully compiled.
  bool GetSuccess() const {
    return module_ && shaderc_module_get_success(module_);
  }

  // Returns any error message found during compilation.
  std::string GetErrorMessage() const {
    if (!module_) {
      return "";
    }
    return shaderc_module_get_error_message(module_);
  }

  // Returns a pointer to the start of the compiled SPIR-V.
  // It is guaranteed that static_cast<uint32_t> is valid to call on this
  // pointer.
  // This pointer remains valid only until the SpvModule is destroyed.
  const char* GetData() const {
    if (!module_) {
      return "";
    }
    return shaderc_module_get_bytes(module_);
  }

  // Returns the number of bytes contained in the compiled SPIR-V.
  size_t GetLength() const {
    if (!module_) {
      return 0;
    }
    return shaderc_module_get_length(module_);
  }

 private:
  SpvModule(const SpvModule& other) = delete;
  SpvModule& operator=(const SpvModule& other) = delete;

  shaderc_spv_module_t module_;
};

// Contains any options that can have default values for a compilation.
class CompileOptions {
 public:
  CompileOptions() { options_ = shaderc_compile_options_initialize(); }
  ~CompileOptions() { shaderc_compile_options_release(options_); }
  CompileOptions(const CompileOptions& other) {
    options_ = shaderc_compile_options_clone(other.options_);
  }
  CompileOptions(CompileOptions&& other) {
    options_ = other.options_;
    other.options_ = nullptr;
  }

 private:
  CompileOptions& operator=(const CompileOptions& other) = delete;
  shaderc_compile_options_t options_;

  friend class Compiler;
};

// The compilation context for compiling source to SPIR-V.
class Compiler {
 public:
  Compiler() : compiler_(shaderc_compiler_initialize()) {}
  ~Compiler() { shaderc_compiler_release(compiler_); }

  Compiler(Compiler&& other) {
    compiler_ = other.compiler_;
    other.compiler_ = nullptr;
  }

  bool IsValid() const { return compiler_ != nullptr; }

  // Compiles the given source GLSL into a SPIR-V module.
  // The source_text parameter must be a valid pointer.
  // The source_text_size parameter must be the length of the source text.
  // The compilation is done with default compile options.
  // It is valid for the returned SpvModule object to outlive this compiler
  // object.
  SpvModule CompileGlslToSpv(const char* source_text, size_t source_text_size,
                             shaderc_shader_kind shader_kind) const {
    shaderc_spv_module_t module = shaderc_compile_into_spv(
        compiler_, source_text, source_text_size, shader_kind, "main", nullptr);
    return SpvModule(module);
  }

  // Compiles the given source GLSL into a SPIR-V module.
  // The source_text parameter must be a valid pointer.
  // The source_text_size parameter must be the length of the source text.
  // The compilation is passed any options specified in the CompileOptions
  // parameter.
  // It is valid for the returned SpvModule object to outlive this compiler
  // object.
  SpvModule CompileGlslToSpv(const char* source_text, size_t source_text_size,
                             shaderc_shader_kind shader_kind,
                             const CompileOptions& options) const {
    shaderc_spv_module_t module =
        shaderc_compile_into_spv(compiler_, source_text, source_text_size,
                                 shader_kind, "main", options.options_);
    return SpvModule(module);
  }

  // Compiles the given source GLSL into a SPIR-V module by invoking
  // CompileGlslToSpv(const char*, size_t, shaderc_shader_kind);
  SpvModule CompileGlslToSpv(const std::string& source_text,
                             shaderc_shader_kind shader_kind) const {
    return CompileGlslToSpv(source_text.data(), source_text.size(),
                            shader_kind);
  }

  // Compiles the given source GLSL into a SPIR-V module by invoking
  // CompileGlslToSpv(const char*, size_t, shaderc_shader_kind, options);
  SpvModule CompileGlslToSpv(const std::string& source_text,
                             shaderc_shader_kind shader_kind,
                             const CompileOptions& options) const {
    return CompileGlslToSpv(source_text.data(), source_text.size(), shader_kind,
                            options);
  }

 private:
  Compiler(const Compiler&) = delete;
  Compiler& operator=(const Compiler& other) = delete;

  shaderc_compiler_t compiler_;
};
};

#endif  // SHADERC_HPP_
