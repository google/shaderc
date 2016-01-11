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

  // Returns the number of warnings generated during the compilation.
  size_t GetNumWarnings() const {
    if (!module_) {
      return 0;
    }
    return shaderc_module_get_num_warnings(module_);
  }

  // Returns the number of errors generated during the compilation.
  size_t GetNumErrors() const {
    if (!module_) {
      return 0;
    }
    return shaderc_module_get_num_errors(module_);
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

  // Adds a predefined macro to the compilation options.
  // For further details refer to shaderc_compile_options_add_macro_definition
  // in shaderc.h
  void AddMacroDefinition(const char* name, const char* value) {
    shaderc_compile_options_add_macro_definition(options_, name, value);
  }

  // Adds a valueless predefined macro to the compilation options.
  void AddMacroDefinition(const std::string& name) {
    AddMacroDefinition(name.c_str(), nullptr);
  }

  // Adds a predefined macro to the compilation options.
  void AddMacroDefinition(const std::string& name, const std::string& value) {
    AddMacroDefinition(name.c_str(), value.c_str());
  }

  // Sets the compiler mode to generate debug information in the output.
  void SetGenerateDebugInfo(){
    shaderc_compile_options_set_generate_debug_info(options_);
  }

  // Sets the compiler to emit a disassembly text instead of a binary. In
  // this mode, the byte array result in the shaderc_spv_module returned
  // from shaderc_compile_into_spv() will consist of SPIR-V assembly text.
  // Note the preprocessing only mode overrides this option, and this option
  // overrides the default mode generating a SPIR-V binary.
  void SetDisassemblyMode(){
    shaderc_compile_options_set_disassembly_mode(options_);
  }

  // Forces the GLSL language version and profile to a given pair. The version
  // number is the same as would appear in the #version annotation in the
  // source. Version and profile specified here overrides the #version
  // annotation in the source. Use profile: 'shaderc_profile_none' for GLSL
  // versions that do not define profiles, e.g. versions below 150.
  void SetForcedVersionProfile(int version, shaderc_profile profile) {
    shaderc_compile_options_set_forced_version_profile(options_, version,
                                                       profile);
  }

  // Sets the compiler to do only preprocessing. The byte array result in the
  // module returned by the compilation is the text of the preprocessed shader.
  // This option overrides all other compilation modes, such as disassembly mode
  // and the default mode of compilation to SPIR-V binary.
  void SetPreprocessingOnlyMode() {
    shaderc_compile_options_set_preprocessing_only_mode(options_);
  }

  // Sets the compiler mode to suppress warnings. Note this option overrides
  // warnings-as-errors mode. When both suppress-warnings and warnings-as-errors
  // modes are turned on, warning messages will be inhibited, and will not be
  // emitted as error message.
  void SetSuppressWarnings() {
    shaderc_compile_options_set_suppress_warnings(options_);
  }

  // Sets the target shader environment, affecting which warnings or errors will be issued.
  // The version will be for distinguishing between different versions of the target environment.
  // "0" is the only supported version at this point
  void SetTargetEnvironment(shaderc_target_env target, uint32_t version) {
    shaderc_compile_options_set_target_env(options_, target, version);
  }

  // Sets the compiler mode to make all warnings into errors. Note the
  // suppress-warnings mode overrides this option, i.e. if both
  // warning-as-errors and suppress-warnings modes are set on, warnings will not
  // be emitted as error message.
  void SetWarningsAsErrors() {
    shaderc_compile_options_set_warnings_as_errors(options_);
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
  // Note when the options_ has disassembly mode or preprocessing only mode set
  // on, the returned SpvModule will hold a text string, instead of a SPIR-V binary
  // generated with default options.
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
