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

#include "compilation_context.h"

#include <cstdint>

#include "libshaderc_util/format.h"
#include "libshaderc_util/io.h"
#include "libshaderc_util/resources.h"
#include "libshaderc_util/string_piece.h"

#include "SPIRV/disassemble.h"
#include "SPIRV/doc.h"
#include "SPIRV/GLSL450Lib.h"
#include "SPIRV/GlslangToSpv.h"

#include "file.h"
#include "message.h"
#include "shader_stage.h"
#include "version_profile.h"

// We must have the following global variable because declared as extern in
// glslang/SPIRV/disassemble.cpp, which we will need for disassembling.
const char* GlslStd450DebugNames[GLSL_STD_450::Count];

namespace {
using shaderc_util::string_piece;

// For use with glslang parsing calls.
const bool kNotForwardCompatible = false;

// Returns true if #line directive sets the line number for the next line in the
// given version and profile.
inline bool LineDirectiveIsForNextLine(int version, EProfile profile) {
  return profile == EEsProfile || version >= 330;
}

}  // anonymous namespace

namespace glslc {
bool CompilationContext::CompileShader(const std::string& input_file,
                                       EShLanguage shader_stage) {
  GlslInitializer initializer;

  std::vector<char> input_data;
  if (!shaderc_util::ReadFile(input_file, &input_data)) {
    return false;
  }

  const std::string output_file = GetOutputFileName(input_file);
  string_piece error_output_file_name(input_file);
  if (input_file == "-") {
    // If the input file was stdin, we want to output errors as <stdin>.
    error_output_file_name = "<stdin>";
  }

  const std::string macro_definitions =
      shaderc_util::format(predefined_macros_, "#define ", " ", "\n");

  std::string preprocessed_shader;

  // If it is preprocess_only_, we definitely need to preprocess. Otherwise, if
  // we don't know the stage until now, we need the preprocessed shader to
  // deduce the shader stage.
  if (preprocess_only_ || shader_stage == EShLangCount) {
    bool success;
    std::string glslang_errors;
    std::tie(success, preprocessed_shader, glslang_errors) =
        PreprocessShader(input_file, input_data, macro_definitions);

    success &= PrintFilteredErrors(error_output_file_name, warnings_as_errors_,
                                   /* suppress_warnings = */ true,
                                   glslang_errors.c_str(), &total_warnings_,
                                   &total_errors_);
    if (!success) return false;

    if (preprocess_only_) {
      return shaderc_util::WriteFile(output_file,
                                     string_piece(preprocessed_shader));
    } else {
      std::string errors;
      std::tie(shader_stage, errors) =
          DeduceShaderStage(input_file, preprocessed_shader);

      if (shader_stage == EShLangCount) {
        std::cerr << errors;
        return false;
      }
    }
  }

  glslang::TShader shader(shader_stage);
  const char* shader_strings = &input_data[0];
  const int shader_lengths = input_data.size();
  shader.setStringsWithLengths(&shader_strings, &shader_lengths, 1);
  shader.setPreamble(macro_definitions.c_str());

  // TODO(dneto): Generate source-level debug info if requested.
  bool success = shader.parse(
      &shaderc_util::kDefaultTBuiltInResource, default_version_,
      default_profile_, force_version_profile_, kNotForwardCompatible,
      EShMsgDefault, glslc::FileIncluder(&include_file_finder_));

  success &= PrintFilteredErrors(error_output_file_name, warnings_as_errors_,
                                 suppress_warnings_, shader.getInfoLog(),
                                 &total_warnings_, &total_errors_);
  if (!success) return false;

  glslang::TProgram program;
  program.addShader(&shader);
  success = program.link(EShMsgDefault);
  success &= PrintFilteredErrors(error_output_file_name, warnings_as_errors_,
                                 suppress_warnings_, program.getInfoLog(),
                                 &total_warnings_, &total_errors_);
  if (!success) return false;

  std::vector<uint32_t> spirv;
  glslang::GlslangToSpv(*program.getIntermediate(shader_stage), spirv);
  if (disassemble_) {
    spv::Parameterize();
    GLSL_STD_450::GetDebugNames(GlslStd450DebugNames);
    std::ostringstream disassembled_spirv;
    spv::Disassemble(disassembled_spirv, spirv);
    return shaderc_util::WriteFile(output_file, disassembled_spirv.str());
  } else {
    return shaderc_util::WriteFile(
        output_file, string_piece(reinterpret_cast<const char*>(spirv.data()),
                                  reinterpret_cast<const char*>(&spirv.back()) +
                                      sizeof(uint32_t)));
  }
}

void CompilationContext::AddIncludeDirectory(const std::string& path) {
  include_file_finder_.search_path().push_back(path);
}

void CompilationContext::AddMacroDefinition(const string_piece& macro,
                                            const string_piece& definition) {
  predefined_macros_[macro] = definition;
}

void CompilationContext::SetForcedVersionProfile(int version,
                                                 EProfile profile) {
  default_version_ = version;
  default_profile_ = profile;
  force_version_profile_ = true;
}
void CompilationContext::SetIndividualCompilationMode() {
  if (!disassemble_) {
    needs_linking_ = false;
    file_extension_ = ".spv";
  }
}
void CompilationContext::SetDisassemblyMode() {
  disassemble_ = true;
  needs_linking_ = false;
  file_extension_ = ".s";
}
void CompilationContext::SetPreprocessingOnlyMode() {
  preprocess_only_ = true;
  needs_linking_ = false;
  if (output_file_name_.empty()) {
    output_file_name_ = "-";
  }
}
void CompilationContext::SetWarningsAsErrors() { warnings_as_errors_ = true; }

void CompilationContext::SetGenerateDebugInfo() { generate_debug_info_ = true; }

void CompilationContext::SetSuppressWarnings() { suppress_warnings_ = true; }

bool CompilationContext::ValidateOptions(size_t num_files) {
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

void CompilationContext::OutputMessages() {
  glslc::OutputMessages(total_warnings_, total_errors_);
}

std::tuple<bool, std::string, std::string> CompilationContext::PreprocessShader(
    const std::string& filename, const std::vector<char>& shader_source,
    const std::string& shader_preamble) {
  const glslc::FileIncluder includer(&include_file_finder_);

  // The stage does not matter for preprocessing.
  glslang::TShader shader(EShLangVertex);
  const char* shader_strings = &shader_source[0];
  const int shader_lengths = shader_source.size();
  shader.setStringsWithLengths(&shader_strings, &shader_lengths, 1);
  shader.setPreamble(shader_preamble.c_str());
  std::string preprocessed_shader;
  const bool success = shader.preprocess(
      &shaderc_util::kDefaultTBuiltInResource, default_version_,
      default_profile_, force_version_profile_, kNotForwardCompatible,
      EShMsgOnlyPreprocessor, &preprocessed_shader, includer);

  if (success) {
    return std::make_tuple(true, preprocessed_shader, shader.getInfoLog());
  }
  return std::make_tuple(false, "", shader.getInfoLog());
}

std::string CompilationContext::GetOutputFileName(std::string input_filename) {
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
  return output_file;
}

std::pair<EShLanguage, std::string> CompilationContext::DeduceShaderStage(
    const std::string& filename, const std::string& preprocessed_shader) {
  EShLanguage stage;
  std::string errors;
  std::string glslang_errors;
  std::tie(stage, errors) =
      GetShaderStageFromSourceCode(filename, preprocessed_shader);
  if (stage == EShLangCount) {
    if (errors.empty()) {
      stage = GetShaderStageFromFileName(filename);
      if (stage == EShLangCount) {
        errors = "glslc: error: '" + filename + "': ";
        if (IsGlslFile(filename)) {
          errors +=
              ".glsl file encountered but no -fshader-stage specified ahead";
        } else if (filename == "-") {
          errors +=
              "-fshader-stage required when input is from standard input \"-\"";
        } else {
          errors += "file not recognized: File format not recognized";
        }
        errors += "\n";
        return std::make_pair(EShLangCount, errors);
      }
    } else {
      return std::make_pair(EShLangCount, errors);
    }
  }
  return std::make_pair(stage, errors);
}

std::pair<EShLanguage, std::string>
CompilationContext::GetShaderStageFromSourceCode(
    const std::string& filename, const std::string& preprocessed_shader) {
  const string_piece kPragmaShaderStageDirective = "#pragma shader_stage";
  const string_piece kLineDirective = "#line";

  int version;
  EProfile profile;
  std::tie(version, profile) = DeduceVersionProfile(preprocessed_shader);
  const bool is_for_next_line = LineDirectiveIsForNextLine(version, profile);

  std::vector<string_piece> lines =
      string_piece(preprocessed_shader).get_fields('\n');
  // The logical line number, which starts from 1 and is sensitive to #line
  // directives, and stage value for #pragma shader_stage() directives.
  std::vector<std::pair<size_t, string_piece>> stages;
  // The physical line numbers of the first #pragma shader_stage() line and
  // first non-preprocessing line in the preprocessed shader text.
  size_t first_pragma_shader_stage = lines.size() + 1;
  size_t first_non_pp_line = lines.size() + 1;

  for (size_t i = 0, logical_line_no = 1; i < lines.size(); ++i) {
    const string_piece current_line = lines[i].strip_whitespace();
    if (current_line.starts_with(kPragmaShaderStageDirective)) {
      const string_piece stage_value =
          current_line.substr(kPragmaShaderStageDirective.size()).strip("()");
      stages.emplace_back(logical_line_no, stage_value);
      first_pragma_shader_stage = std::min(first_pragma_shader_stage, i + 1);
    } else if (!current_line.empty() && !current_line.starts_with("#")) {
      first_non_pp_line = std::min(first_non_pp_line, i + 1);
    }

    // Update logical line number for the next line.
    if (current_line.starts_with(kLineDirective)) {
      // Note that for core profile, the meaning of #line changed since version
      // 330. The line number given by #line used to mean the logical line
      // number of the #line line. Now it means the logical line number of the
      // next line after the #line line.
      logical_line_no =
          std::atoi(current_line.substr(kLineDirective.size()).data()) +
          (is_for_next_line ? 0 : 1);
    } else {
      ++logical_line_no;
    }
  }
  if (stages.empty()) return std::make_pair(EShLangCount, "");

  std::string error_message;

  // TODO(antiagainst): #line could change the effective filename once we add
  // support for filenames in #line directives.

  if (first_pragma_shader_stage > first_non_pp_line) {
    error_message += filename + ":" + std::to_string(stages[0].first) +
                     ": error: '#pragma': the first 'shader_stage' #pragma "
                     "must appear before any non-preprocessing code\n";
  }

  EShLanguage stage = MapStageNameToLanguage(stages[0].second);
  if (stage == EShLangCount) {
    error_message +=
        filename + ":" + std::to_string(stages[0].first) +
        ": error: '#pragma': invalid stage for 'shader_stage' #pragma: '" +
        stages[0].second.str() + "'\n";
  }

  for (size_t i = 1; i < stages.size(); ++i) {
    if (stages[i].second != stages[0].second) {
      error_message += filename + ":" + std::to_string(stages[i].first) +
                       ": error: '#pragma': conflicting stages for "
                       "'shader_stage' #pragma: '" +
                       stages[i].second.str() + "' (was '" +
                       stages[0].second.str() + "' at " + filename + ":" +
                       std::to_string(stages[0].first) + ")\n";
    }
  }

  return std::make_pair(error_message.empty() ? stage : EShLangCount,
                        error_message);
}

std::pair<int, EProfile> CompilationContext::DeduceVersionProfile(
    const std::string& preprocessed_shader) {
  int version = default_version_;
  EProfile profile = default_profile_;
  if (!force_version_profile_) {
    std::tie(version, profile) =
        GetVersionProfileFromSourceCode(preprocessed_shader);
    if (version == 0 && profile == ENoProfile) {
      version = default_version_;
      profile = default_profile_;
    }
  }
  return std::make_pair(version, profile);
}

std::pair<int, EProfile> CompilationContext::GetVersionProfileFromSourceCode(
    const std::string& preprocessed_shader) {
  string_piece pound_version = preprocessed_shader;
  const size_t pound_version_loc = pound_version.find("#version");
  if (pound_version_loc == string_piece::npos) {
    return std::make_pair(0, ENoProfile);
  }
  pound_version =
      pound_version.substr(pound_version_loc + std::strlen("#version"));
  pound_version = pound_version.substr(0, pound_version.find_first_of("\n"));

  std::string version_profile;
  for (const auto character : pound_version) {
    if (character != ' ') version_profile += character;
  }

  int version;
  EProfile profile;
  if (!glslc::GetVersionProfileFromCmdLine(version_profile, &version,
                                           &profile)) {
    return std::make_pair(0, ENoProfile);
  }
  return std::make_pair(version, profile);
}

}  // namesapce glslc
