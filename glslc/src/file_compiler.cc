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

#include "file_compiler.h"

#include <fstream>
#include <iostream>

#include "file.h"
#include "file_includer.h"
#include "shader_stage.h"

#include "libshaderc_util/io.h"
#include "libshaderc_util/message.h"

namespace {
using shaderc_util::string_piece;

}  // anonymous namespace

namespace glslc {
bool FileCompiler::CompileShaderFile(const std::string& input_file,
                                     shaderc_shader_kind shader_stage) {
  std::vector<char> input_data;
  std::string path = input_file;
  if (!workdir_.empty() && !shaderc_util::IsAbsolutePath(path)) {
    path = workdir_ + path;
  }
  if (!shaderc_util::ReadFile(path, &input_data)) {
    return false;
  }

  std::string output_name = GetOutputFileName(input_file);

  std::ofstream potential_file_stream;
  std::ostream* output_stream =
      shaderc_util::GetOutputStream(output_name, &potential_file_stream);
  if (!output_stream) {
    // An error message has already been emitted to the stderr stream.
    return false;
  }
  string_piece error_file_name = input_file;

  if (error_file_name == "-") {
    // If the input file was stdin, we want to output errors as <stdin>.
    error_file_name = "<stdin>";
  }

  string_piece source_string;
  if (!input_data.empty()) {
    source_string = string_piece(&(*input_data.begin()),
                                 &(*input_data.begin()) + input_data.size());
  }

  std::unique_ptr<FileIncluder> includer(new FileIncluder(&include_file_finder_));
  options_.SetIncluder(std::move(includer));

  shaderc::SpvModule result =
      compiler_.CompileGlslToSpv(source_string.begin(), source_string.size(),
                                 shader_stage, error_file_name.data(), options_);
  total_errors_ += result.GetNumErrors();
  total_warnings_ += result.GetNumWarnings();

  bool compilation_success =
      result.GetCompilationStatus() == shaderc_compilation_status_success;

  // Write output to output file.
  output_stream->write(result.GetData(), result.GetLength());
  // Write error message to std::cerr.
  std::cerr << result.GetErrorMessage();
  // Something wrong happen for output.
  if (output_stream->fail()) {
    if (output_stream == &std::cout) {
      std::cerr << "glslc: error: error writing to standard output"
                << std::endl;
    } else {
      std::cerr << "glslc: error: error writing to output file: '"
                << output_file_name_ << "'" << std::endl;
    }
  }

  // Handle the error message for failing to deduce the shader kind.
  if (result.GetCompilationStatus() ==
      shaderc_compilation_status_invalid_stage) {
    if (IsGlslFile(error_file_name)) {
      std::cerr << "glslc: error: "
                << "'" << error_file_name << "': "
                << ".glsl file encountered but no -fshader-stage "
                   "specified ahead";
    } else if (error_file_name == "<stdin>") {
      std::cerr
          << "glslc: error: '-': -fshader-stage required when input is from "
             "standard "
             "input \"-\"";
    } else {
      std::cerr << "glslc: error: "
                << "'" << error_file_name << "': "
                << "file not recognized: File format not recognized";
    }
    std::cerr << "\n";
  }

  return compilation_success;
}

void FileCompiler::SetWorkingDirectory(const std::string& dir) {
  workdir_ = dir;
  if (!dir.empty() && dir.back() != '/') workdir_.push_back('/');
}

void FileCompiler::AddIncludeDirectory(const std::string& path) {
  include_file_finder_.search_path().push_back(path);
}

void FileCompiler::SetIndividualCompilationFlags() {
  if (!disassemble_) {
    needs_linking_ = false;
    file_extension_ = ".spv";
  }
}

void FileCompiler::SetDisassemblyFlags() {
  disassemble_ = true;
  needs_linking_ = false;
  file_extension_ = ".s";
}

void FileCompiler::SetPreprocessingOnlyFlags() {
  preprocess_only_ = true;
  needs_linking_ = false;
  if (output_file_name_.empty()) {
    output_file_name_ = "-";
  }
}

bool FileCompiler::ValidateOptions(size_t num_files) {
  if (num_files == 0) {
    std::cerr << "glslc: error: no input files" << std::endl;
    return false;
  }

  if (num_files > 1 && needs_linking_) {
    std::cerr << "glslc: error: linking multiple files is not supported yet. "
                 "Use -c to compile files individually."
              << std::endl;
    return false;
  }

  // If we are outputting many object files, we cannot specify -o. Also
  // if we are preprocessing multiple files they must be to stdout.
  if (num_files > 1 &&
      ((!preprocess_only_ && !needs_linking_ && !output_file_name_.empty()) ||
       (preprocess_only_ && output_file_name_ != "-"))) {
    std::cerr << "glslc: error: cannot specify -o when generating multiple"
                 " output files"
              << std::endl;
    return false;
  }
  return true;
}

void FileCompiler::OutputMessages() {
  shaderc_util::OutputMessages(&std::cerr, total_warnings_, total_errors_);
}

std::string FileCompiler::GetOutputFileName(std::string input_filename) {
  std::string output_file = "a.spv";
  if (!needs_linking_) {
    if (IsStageFile(input_filename)) {
      output_file = input_filename + file_extension_;
    } else {
      output_file = input_filename.substr(0, input_filename.find_last_of('.')) +
                    file_extension_;
    }
  }
  if (!output_file_name_.empty()) output_file = output_file_name_.str();
  if (!needs_linking_ && !workdir_.empty() &&
      !shaderc_util::IsAbsolutePath(input_filename)) {
    output_file = workdir_ + output_file;
  }
  return output_file;
}

}  // namesapce glslc
