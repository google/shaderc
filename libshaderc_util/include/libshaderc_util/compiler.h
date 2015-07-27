//
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

#ifndef LIBSHADERC_UTIL_INC_COMPILER_H
#define LIBSHADERC_UTIL_INC_COMPILER_H

#include <functional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>

#include "glslang/Public/ShaderLang.h"

#include "counting_includer.h"
#include "file_finder.h"
#include "string_piece.h"

namespace shaderc_util {

// Initializes glslang on creation, and destroys it on completion.
class GlslInitializer {
 public:
  GlslInitializer() { glslang::InitializeProcess(); }
  ~GlslInitializer() { glslang::FinalizeProcess(); }
};

// Maps macro names to their definitions.  Stores string_pieces, so the
// underlying strings must outlive it.
using MacroDictionary =
    std::unordered_map<shaderc_util::string_piece, shaderc_util::string_piece>;

// Holds all of the state required to compile source GLSL into SPIR-V.
class Compiler {
 public:
  Compiler()
      : default_version_(110),
        default_profile_(ENoProfile),
        warnings_as_errors_(false),
        disassemble_(false),
        force_version_profile_(false),
        preprocess_only_(false),
        generate_debug_info_(false),
        suppress_warnings_(false) {}

  // Instead of outputting object files, output the disassembled textual output.
  virtual void SetDisassemblyMode();

  // Instead of outputting object files, output the preprocessed source files.
  virtual void SetPreprocessingOnlyMode();

  // Requests that the compiler place debug information into the object code,
  // such as identifier names and line numbers.
  void SetGenerateDebugInfo();

  // When a warning is encountered it treat it as an error.
  void SetWarningsAsErrors();

  // Any warning message generated is suppressed before it is output.
  void SetSuppressWarnings();

  // Adds an implicit macro definition obeyed by subsequent CompileShader()
  // calls.
  void AddMacroDefinition(const string_piece& macro,
                          const string_piece& definition);

  // Forces (without any verification) the default version and profile for
  // subsequent CompileShader() calls.
  void SetForcedVersionProfile(int version, EProfile profile);

  // Compiles the shader source in the input_source_string parameter.
  // The compiled SPIR-V is written to output_stream.
  //
  // If the forced_shader stage parameter is not EShLangCount then
  // the shader is assumed to be of the given stage.
  //
  // The stage_callback function will be called if a shader_stage has
  // not been forced and the stage can not be determined
  // from the shader text. Any #include directives are parsed with the given
  // includer.
  //
  // Any error messages are written as if the file name were error_tag.
  // Any errors are written to the error_stream parameter.
  // total_warnings and total_errors are incremented once for every
  // warning or error encountered respectively.
  // Returns true if the compilation succeeded and the result could be written
  // to output, false otherwise.
  bool Compile(const shaderc_util::string_piece& input_source_string,
               EShLanguage forced_shader_stage, const std::string& error_tag,
               const std::function<EShLanguage(std::ostream* error_stream,
                                               const shaderc_util::string_piece&
                                                   error_tag)>& stage_callback,
               const CountingIncluder& includer, std::ostream* output_stream,
               std::ostream* error_stream, size_t* total_warnings,
               size_t* total_errors) const;

 protected:
  // Preprocesses a shader whose filename is filename and content is
  // shader_source. If preprocessing is successful, returns true, the
  // preprocessed shader, and any warning message as a tuple. Otherwise,
  // returns false, an empty string, and error messages as a tuple.
  //
  // The error_tag parameter is the name to use for outputting errors.
  // The shader_source parameter is the input shader's source text.
  // The shader_preamble parameter is a context-specific preamble internally
  // prepended to shader_text without affecting the validity of its #version
  // position.
  //
  // Any #include directives are processed with the given includer.
  //
  // If force_version_profile_ is set, the shader's version/profile is forced
  // to be default_version_/default_profile_ regardless of the #version
  // directive in the source code.
  std::tuple<bool, std::string, std::string> PreprocessShader(
      const std::string& error_tag,
      const shaderc_util::string_piece& shader_source,
      const shaderc_util::string_piece& shader_preamble,
      const CountingIncluder& includer) const;

  // Cleans up the preamble in a given preprocessed shader.
  //
  // The error_tag parameter is the name to be given for the main file.
  // The pound_extension parameter is the #extension directive we prepended to
  // the original shader source code via preamble.
  // The num_include_directives parameter is the number of #include directives
  // appearing in the original shader source code.
  // The is_for_next_line means whether the #line sets the line number for the
  // next line.
  //
  // If no #include directive is used in the shader source code, we can safely
  // delete the #extension directive we injected via preamble. Otherwise, we
  // need to adjust it if there exists a #version directive in the original
  // shader source code.
  std::string CleanupPreamble(
      const shaderc_util::string_piece& preprocessed_shader,
      const shaderc_util::string_piece& error_tag,
      const shaderc_util::string_piece& pound_extension,
      int num_include_directives, bool is_for_next_line) const;

  // Determines version and profile from command line, or the source code.
  // Returns the decoded version and profile pair on success. Otherwise,
  // returns (0, ENoProfile).
  std::pair<int, EProfile> DeduceVersionProfile(
      const std::string& preprocessed_shader) const;

  // Determines the shader stage from pragmas embedded in the source text if
  // possible. In the returned pair, the glslang EShLanguage is the shader
  // stage deduced. If no #pragma directives for shader stage exist, it's
  // EShLangCount.  If errors occur, the second element in the pair is the
  // error message.  Otherwise, it's an empty string.
  std::pair<EShLanguage, std::string> GetShaderStageFromSourceCode(
      const shaderc_util::string_piece& filename,
      const std::string& preprocessed_shader) const;

  // Determines version and profile from command line, or the source code.
  // Returns the decoded version and profile pair on success. Otherwise,
  // returns (0, ENoProfile).
  std::pair<int, EProfile> DeduceVersionProfile(
      const std::string& preprocessed_shader);

  // Gets version and profile specification from the given preprocessedshader.
  // Returns the decoded version and profile pair on success. Otherwise,
  // returns (0, ENoProfile).
  std::pair<int, EProfile> GetVersionProfileFromSourceCode(
      const std::string& preprocessed_shader) const;

  // The default version for glsl is 110, or 100 if you are using an
  // es profile. But we want to default to a non-es profile.
  int default_version_;

  EProfile default_profile_;
  bool warnings_as_errors_;
  bool disassemble_;
  bool force_version_profile_;
  bool preprocess_only_;
  bool generate_debug_info_;
  bool suppress_warnings_;
  MacroDictionary predefined_macros_;
};
}  // namespace shaderc_util
#endif  // LIBSHADERC_UTIL_INC_COMPILER_H
