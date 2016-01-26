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

#ifndef GLSLC_FILE_COMPILER_H
#define GLSLC_FILE_COMPILER_H

#include <string>

#include "libshaderc_util/string_piece.h"
#include "libshaderc_util/file_finder.h"
#include "shaderc.hpp"

namespace glslc {

// Context for managing compilation of source GLSL files into destination
// SPIR-V files.
class FileCompiler {
 public:
  FileCompiler() : needs_linking_(true), total_warnings_(0), total_errors_(0) {}

  // Compiles a shader received in input_file, returning true on success and
  // false otherwise. If force_shader_stage is not shaderc_glsl_infer_source or
  // any default shader stage then the given shader_stage will be used, otherwise
  // it will be determined from the source or the file type.
  //
  // Places the compilation output into a new file whose name is derived from
  // input_file according to the rules from glslc/README.asciidoc.
  //
  // If version/profile has been forced, the shader's version/profile is set to
  // that value regardless of the #version directive in the source code.
  //
  // Any errors/warnings found in the shader source will be output to std::cerr
  // and increment the counts reported by OutputMessages().
  bool CompileShaderFile(const std::string& input_file,
                         shaderc_shader_kind shader_stage);

  // Sets the working directory for the compilation.
  void SetWorkingDirectory(const std::string& dir);

  // Adds a directory to be searched when processing #include directives.
  //
  // Best practice: if you add an empty string before any other path, that will
  // correctly resolve both absolute paths and paths relative to the current
  // working directory.
  void AddIncludeDirectory(const std::string& path);

  // Sets the output filename. A name of "-" indicates standard output.
  void SetOutputFileName(const shaderc_util::string_piece& file) {
    output_file_name_ = file;
  }

  // Returns false if any options are incompatible. The num_files parameter
  // represents the number of files that will be compiled.
  bool ValidateOptions(size_t num_files);

  // Outputs to std::cerr the number of warnings and errors if there are any.
  void OutputMessages();

  // Sets the flag to indicate individual compilation mode. In this mode, all
  // files are compiled individually and written to separate output files
  // instead of linked together. This method also sets the needs_linking_ flags
  // to false and the file_extension to ".spv". Disassembly mode and preprocessing
  // only mode override this mode and flags.
  void SetIndividualCompilationFlags();

  // Sets the flag to indicate disassembly mode. In this mode, the compiler
  // emits disassembled textual output, instead of outputting object files.
  // This method also sets the file_extension to ".s" and needs_linking_ flag to
  // false. This mode overrides individual compilation mode, and preprocessing
  // only mode overrides this mode.
  void SetDisassemblyFlags();

  // Sets the flag to indicate preprocessing only mode. In this mode, instead of
  // outputting object files, the compiler emits the preprocessed source files.
  // This method also sets the needs_linking_ flag to false and set the output
  // file to stdout. This mode overrides disassembly mode and individual
  // compilation mode.
  void SetPreprocessingOnlyFlags();

  // Gets the reference of the compiler options which reflects the command-line
  // arguments.
  shaderc::CompileOptions& options() {return options_;};

 private:
  // Performs actual SPIR-V compilation on the contents of input files.
  shaderc::Compiler compiler_;

  // Reflects the command-line arguments and goes into
  // compiler_.CompileGlslToSpv().
  shaderc::CompileOptions options_;

  // Returns the name of the output file, given the input_filename string.
  std::string GetOutputFileName(std::string input_filename);

  // Resolves relative paths against this working directory. Must always end
  // in '/'.
  std::string workdir_;

  // A FileFinder used to substitute #include directives in the source code.
  shaderc_util::FileFinder include_file_finder_;

  // Indicates whether linking is needed to generate the final output.
  bool needs_linking_;

  // Flag for disassembly mode.
  bool disassemble_ = false;

  // Flag for preprocessing only mode.
  bool preprocess_only_ = false;

  // Reflects the type of file being generated.
  std::string file_extension_;
  // Name of the file where the compilation output will go.
  shaderc_util::string_piece output_file_name_;

  // Counts warnings encountered in compilation.
  size_t total_warnings_;
  // Counts errors encountered in compilation.
  size_t total_errors_;
};
}
#endif  // GLSLC_FILE_COMPILER_H
