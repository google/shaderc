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

#ifndef GLSLC_COMPILATION_CONTEXT_H
#define GLSLC_COMPILATION_CONTEXT_H

#include <functional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>

#include "glslang/Public/ShaderLang.h"

#include "libshaderc_util/file_finder.h"
#include "libshaderc_util/string_piece.h"

#include "file_includer.h"

namespace glslc {

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

class CompilationContext {
 public:
  CompilationContext()
      : default_version_(110),
        default_profile_(ENoProfile),
        warnings_as_errors_(false),
        disassemble_(false),
        force_version_profile_(false),
        preprocess_only_(false),
        needs_linking_(true),
        generate_debug_info_(false),
        suppress_warnings_(false),
        total_warnings_(0),
        total_errors_(0) {}

  // Compiles a shader received in input_file, returning true on success and
  // false otherwise. If force_shader_stage is not EShLangCount then the
  // given shader_stage will be used, otherwise it will be determined
  // from the source or the file type.
  //
  // Places the compilation output into a new file whose name is derived from
  // input_file according to the rules from glslc/README.asciidoc.
  //
  // If version/profile has been forced, the shader's version/profile is set to
  // that value regardless of the #version directive in the source code.
  //
  // Any errors/warnings found in the shader source will be output to std::cerr
  // and increment the counts reported by OutputMessages().
  bool CompileShader(const std::string& input_file, EShLanguage shader_stage);

  // Adds a directory to be searched when processing #include directives.
  //
  // Best practice: if you add an empty string before any other path, that will
  // correctly resolve both absolute paths and paths relative to the current
  // working directory.
  void AddIncludeDirectory(const std::string& path);

  // Adds an implicit macro definition obeyed by subsequent CompileShader()
  // calls.
  void AddMacroDefinition(const shaderc_util::string_piece& macro,
                          const shaderc_util::string_piece& definition);

  // Forces (without any verification) the default version and profile for
  // subsequent CompileShader() calls.
  void SetForcedVersionProfile(int version, EProfile profile);

  // Sets the output filename. A name of "-" indicates standard output.
  void SetOutputFileName(const shaderc_util::string_piece& file) {
    output_file_name_ = file;
  }

  // When any files are to be compiled, they are compiled individually and
  // written to separate output files instead of linked together.
  void SetIndividualCompilationMode();

  // Instead of outputting object files, output the disassembled textual output.
  void SetDisassemblyMode();

  // Instead of outputting object files, output the preprocessed source files.
  void SetPreprocessingOnlyMode();

  // Requests that the compiler place debug information into the object code,
  // such as identifier names and line numbers.
  void SetGenerateDebugInfo();

  // When a warning is encountered it treat it as an error.
  void SetWarningsAsErrors();

  // Any warning message generated is suppressed before it is output.
  void SetSuppressWarnings();

  // Returns false if any options are incompatible.  The num_files parameter
  // represents the number of files that will be compiled.
  bool ValidateOptions(size_t num_files);

  // Outputs to std::cerr the number of warnings and errors if there are any.
  void OutputMessages();

 private:
  // Compiles the shader source in the input_source_string parameter.
  // The compiled SPIR-V is written to output_stream.
  //
  // The input_filename parameter is the filename of the input shader.
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
  // Returns true if the compilation succeeded and the result could be written
  // to output, false otherwise.
  bool DoCompilation(
      const std::string& input_filename,
      const shaderc_util::string_piece& input_source_string,
      EShLanguage forced_shader_stage,
      const shaderc_util::string_piece& error_tag,
      const std::function<EShLanguage(
          std::ostream* error_stream,
          const shaderc_util::string_piece& error_tag)>& stage_callback,
      const glslang::TShader::Includer& includer, std::ostream* output_stream,
      std::ostream* error_stream);
  // Preprocesses a shader whose filename is filename and content is
  // shader_source. If preprocessing is successful, returns true, the
  // preprocessed shader, and any warning message as a tuple. Otherwise,
  // returns false, an empty string, and error messages as a tuple.
  //
  // The filename parameter is the input shader's filename.
  // The shader_source parameter is the input shader's source text.
  // The shader_preamble parameter is a context-specific preamble internally
  // prepended to shader_text without affecting the validity of its #version
  // position.
  //
  // If force_version_profile_ is set, the shader's version/profile is forced
  // to be default_version_/default_profile_ regardless of the #version
  // directive in the source code.
  std::tuple<bool, std::string, std::string> PreprocessShader(
      const std::string& filename,
      const shaderc_util::string_piece& shader_source,
      const std::string& shader_preamble, const glslc::FileIncluder& includer);

  // Cleans up the preamble in a given preprocessed shader.
  //
  // The main_file parameter is the name of the main source file.
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
      const std::string& main_file,
      const shaderc_util::string_piece& pound_extension,
      int num_include_directives, bool is_for_next_line);

  // Returns the name of the output file, given the input_filename string.
  std::string GetOutputFileName(std::string input_filename);

  // Determines the shader stage from pragmas embedded in the source text or
  // from the filename extension, if possible. If errors occur, returns an
  // errors string in the second element of the pair and returns EShLangCount
  // in the first element.
  std::pair<EShLanguage, std::string> DeduceShaderStage(
      const std::string& filename, const std::string& preprocessed_shader);

  // Determines the shader stage from pragmas embedded in the source text if
  // possible. In the returned pair, the glslang EShLanguage is the shader
  // stage deduced. If no #pragma directives for shader stage exist, it's
  // EShLangCount.  If errors occur, the second element in the pair is the
  // error message.  Otherwise, it's an empty string.
  std::pair<EShLanguage, std::string> GetShaderStageFromSourceCode(
      const shaderc_util::string_piece& filename,
      const std::string& preprocessed_shader);

  // Determines version and profile from command line, or the source code.
  // Returns the decoded version and profile pair on success. Otherwise,
  // returns (0, ENoProfile).
  std::pair<int, EProfile> DeduceVersionProfile(
      const std::string& preprocessed_shader);

  // Gets version and profile specification from the given preprocessedshader.
  // Returns the decoded version and profile pair on success. Otherwise,
  // returns (0, ENoProfile).
  std::pair<int, EProfile> GetVersionProfileFromSourceCode(
      const std::string& preprocessed_shader);

  // The default version for glsl is 110, or 100 if you are using an
  // es profile. But we want to default to a non-es profile.
  int default_version_;

  EProfile default_profile_;
  bool warnings_as_errors_;
  bool disassemble_;
  bool force_version_profile_;
  bool preprocess_only_;
  bool needs_linking_;
  bool generate_debug_info_;
  bool suppress_warnings_;
  MacroDictionary predefined_macros_;
  shaderc_util::FileFinder include_file_finder_;

  // This is set by the various Set*Mode functions. It is set to reflect the
  // type of file being generated.
  std::string file_extension_;
  shaderc_util::string_piece output_file_name_;
  size_t total_warnings_;
  size_t total_errors_;
};
}
#endif  // GLSLC_COMPILATION_CONTEXT_H
