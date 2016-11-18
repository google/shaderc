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

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <list>
#include <string>
#include <utility>

#include "libshaderc_util/compiler.h"
#include "libshaderc_util/io.h"
#include "libshaderc_util/string_piece.h"
#include "shaderc/shaderc.h"
#include "spirv-tools/libspirv.h"

#include "file.h"
#include "file_compiler.h"
#include "resource_parse.h"
#include "shader_stage.h"

using shaderc_util::string_piece;

namespace {

// Prints the help message.
void PrintHelp(std::ostream* out) {
  *out << R"(glslc - Compile shaders into SPIR-V

Usage: glslc [options] file...

An input file of - represents standard input.

Options:
  -c                Only run preprocess, compile, and assemble steps.
  -Dmacro[=defn]    Add an implicit macro definition.
  -E                Outputs only the results of the preprocessing step.
                    Output defaults to standard output.
  -fentry-point=<name>
                    Specify the entry point name for HLSL compilation, for
                    all subsequent source files.  Default is "main".
  -flimit=<settings>
                    Specify resource limits. Each limit is specified by a limit
                    name followed by an integer value.  Tokens should be
                    separated by whitespace.  If the same limit is specified
                    several times, only the last setting takes effect.
  --show-limits     Display available limit names and their default values.
  -flimit-file <file>
                    Set limits as specified in the given file.
  -fshader-stage=<stage>
                    Treat subsequent input files as having stage <stage>.
                    Valid stages are vertex, fragment, tesscontrol, tesseval,
                    geometry, and compute.
  -g                Generate source-level debug information.
                    Currently this option has no effect.
  --help            Display available options.
  --version         Display compiler version information.
  -I <value>        Add directory to include search path.
  -o <file>         Write output to <file>.
                    A file name of '-' represents standard output.
  -std=<value>      Version and profile for GLSL input files. Possible values
                    are concatenations of version and profile, e.g. 310es,
                    450core, etc.  Ignored for HLSL files.
  -mfmt=<format>    Output SPIR-V binary code using the selected format. This
                    option may be specified only when the compilation output is
                    in SPIR-V binary code form. Available options include bin, c
                    and num. By default the binary output format is bin.
  -M                Generate make dependencies. Implies -E and -w.
  -MM               An alias for -M.
  -MD               Generate make dependencies and compile.
  -MF <file>        Write dependency output to the given file.
  -MT <target>      Specify the target of the rule emitted by dependency
                    generation.
  -S                Only run preprocess and compilation steps.
  --target-env=<environment>
                    Set the target shader environment, and the semantics
                    of warnings and errors. Valid values are 'opengl',
                    'opengl_compat' and 'vulkan'. The default value is 'vulkan'.
  -w                Suppresses all warning messages.
  -Werror           Treat all warnings as errors.
  -x <language>     Treat subsequent input files as having type <language>.
                    Valid languages are: glsl, hlsl.
                    For files ending in .hlsl the default is hlsl.
                    Otherwise the default is glsl.
)";
}

// Gets the option argument for the option at *index in argv in a way consistent
// with clang/gcc. On success, returns true and writes the parsed argument into
// *option_argument. Returns false if any errors occur. After calling this
// function, *index will the index of the last command line argument consumed.
bool GetOptionArgument(int argc, char** argv, int* index,
                       const std::string& option,
                       string_piece* option_argument) {
  const string_piece arg = argv[*index];
  assert(arg.starts_with(option));
  if (arg.size() != option.size()) {
    *option_argument = arg.substr(option.size());
    return true;
  } else {
    if (option.back() == '=') {
      *option_argument = "";
      return true;
    }
    if (++(*index) >= argc) return false;
    *option_argument = argv[*index];
    return true;
  }
}

// Sets resource limits according to the given string. The string
// should be formated as required for ParseResourceSettings.
// Returns true on success.  Otherwise returns false and sets err
// to a descriptive error message.
bool SetResourceLimits(const std::string& str, shaderc::CompileOptions* options,
                       std::string* err) {
  std::vector<glslc::ResourceSetting> settings;
  if (!ParseResourceSettings(str, &settings, err)) {
    return false;
  }
  for (const auto& setting : settings) {
    options->SetLimit(setting.limit, setting.value);
  }
  return true;
}

const char kBuildVersion[] =
#include "build-version.inc"
    ;
}  // anonymous namespace

int main(int argc, char** argv) {
  std::vector<glslc::InputFileSpec> input_files;
  shaderc_shader_kind current_fshader_stage = shaderc_glsl_infer_from_source;
  bool source_language_forced = false;
  shaderc_source_language current_source_language =
      shaderc_source_language_glsl;
  std::string current_entry_point_name("main");
  glslc::FileCompiler compiler;
  bool success = true;
  bool has_stdin_input = false;

  for (int i = 1; i < argc; ++i) {
    const string_piece arg = argv[i];
    if (arg == "--help") {
      ::PrintHelp(&std::cout);
      return 0;
    } else if (arg == "--show-limits") {
      shaderc_util::Compiler default_compiler;
// The static cast here depends on us keeping the shaderc_limit enum in
// lockstep with the shaderc_util::Compiler::Limit enum.  The risk of mismatch
// is low since both are generated from the same resources.inc file.
#define RESOURCE(NAME, FIELD, ENUM)                            \
  std::cout << #NAME << " "                                    \
            << default_compiler.GetLimit(                      \
                   static_cast<shaderc_util::Compiler::Limit>( \
                       shaderc_limit_##ENUM))                  \
            << std::endl;
#include "libshaderc_util/resources.inc"
#undef RESOURCE
      return 0;
    } else if (arg == "--version") {
      std::cout << kBuildVersion << std::endl;
      std::cout << "Target: " << spvTargetEnvDescription(SPV_ENV_UNIVERSAL_1_0)
                << std::endl;
      return 0;
    } else if (arg.starts_with("-o")) {
      string_piece file_name;
      if (!GetOptionArgument(argc, argv, &i, "-o", &file_name)) {
        std::cerr
            << "glslc: error: argument to '-o' is missing (expected 1 value)"
            << std::endl;
        return 1;
      }
      compiler.SetOutputFileName(file_name);
    } else if (arg.starts_with("-fshader-stage=")) {
      const string_piece stage = arg.substr(std::strlen("-fshader-stage="));
      current_fshader_stage = glslc::GetForcedShaderKindFromCmdLine(arg);
      if (current_fshader_stage == shaderc_glsl_infer_from_source) {
        std::cerr << "glslc: error: stage not recognized: '" << stage << "'"
                  << std::endl;
        return 1;
      }
    } else if (arg.starts_with("-fentry-point=")) {
      current_entry_point_name =
          arg.substr(std::strlen("-fentry-point=")).str();
    } else if (arg.starts_with("-flimit=")) {
      std::string err;
      if (!SetResourceLimits(arg.substr(std::strlen("-flimit=")).str(),
                             &compiler.options(), &err)) {
        std::cerr << "glslc: error: -flimit error: " << err << std::endl;
        return 1;
      }
    } else if (arg.starts_with("-flimit-file")) {
      std::string err;
      string_piece limits_file;
      if (!GetOptionArgument(argc, argv, &i, "-flimit-file", &limits_file)) {
        std::cerr << "glslc: error: argument to '-flimit-file' is missing"
                  << std::endl;
        return 1;
      }
      std::vector<char> contents;
      if (!shaderc_util::ReadFile(limits_file.str(), &contents)) {
        std::cerr << "glslc: cannot read limits file: " << limits_file
                  << std::endl;
        return 1;
      }
      if (!SetResourceLimits(
              string_piece(contents.data(), contents.data() + contents.size())
                  .str(),
              &compiler.options(), &err)) {
        std::cerr << "glslc: error: -flimit-file error: " << err << std::endl;
        return 1;
      }
    } else if (arg.starts_with("-std=")) {
      const string_piece standard = arg.substr(std::strlen("-std="));
      int version;
      shaderc_profile profile;
      if (!shaderc_parse_version_profile(standard.begin(), &version,
                                         &profile)) {
        std::cerr << "glslc: error: invalid value '" << standard
                  << "' in '-std=" << standard << "'" << std::endl;
        return 1;
      }
      compiler.options().SetForcedVersionProfile(version, profile);
    } else if (arg.starts_with("--target-env=")) {
      shaderc_target_env target_env = shaderc_target_env_default;
      const string_piece target_env_str =
          arg.substr(std::strlen("--target-env="));
      if (target_env_str == "vulkan") {
        target_env = shaderc_target_env_vulkan;
      } else if (target_env_str == "opengl") {
        target_env = shaderc_target_env_opengl;
      } else if (target_env_str == "opengl_compat") {
        target_env = shaderc_target_env_opengl_compat;
      } else {
        std::cerr << "glslc: error: invalid value '" << target_env_str
                  << "' in '--target-env=" << target_env_str << "'"
                  << std::endl;
        return 1;
      }
      compiler.options().SetTargetEnvironment(target_env, 0);
    } else if (arg.starts_with("-mfmt=")) {
      const string_piece binary_output_format =
          arg.substr(std::strlen("-mfmt="));
      if (binary_output_format == "bin") {
        compiler.SetSpirvBinaryOutputFormat(
            glslc::FileCompiler::SpirvBinaryEmissionFormat::Binary);
      } else if (binary_output_format == "num") {
        compiler.SetSpirvBinaryOutputFormat(
            glslc::FileCompiler::SpirvBinaryEmissionFormat::Numbers);
      } else if (binary_output_format == "c") {
        compiler.SetSpirvBinaryOutputFormat(
            glslc::FileCompiler::SpirvBinaryEmissionFormat::CInitList);
      } else {
        std::cerr << "glslc: error: invalid value '" << binary_output_format
                  << "' in '-mfmt=" << binary_output_format << "'" << std::endl;
        return 1;
      }
    } else if (arg.starts_with("-x")) {
      string_piece option_arg;
      if (!GetOptionArgument(argc, argv, &i, "-x", &option_arg)) {
        std::cerr
            << "glslc: error: argument to '-x' is missing (expected 1 value)"
            << std::endl;
        success = false;
      } else {
        if (option_arg == "glsl") {
          current_source_language = shaderc_source_language_glsl;
        } else if (option_arg == "hlsl") {
          current_source_language = shaderc_source_language_hlsl;
        } else {
          std::cerr << "glslc: error: language not recognized: '" << option_arg
                    << "'" << std::endl;
          return 1;
        }
        source_language_forced = true;
      }
    } else if (arg == "-c") {
      compiler.SetIndividualCompilationFlag();
    } else if (arg == "-E") {
      compiler.SetPreprocessingOnlyFlag();
    } else if (arg == "-M" || arg == "-MM") {
      // -M implies -E and -w
      compiler.SetPreprocessingOnlyFlag();
      compiler.options().SetSuppressWarnings();
      if (compiler.GetDependencyDumpingHandler()->DumpingModeNotSet()) {
        compiler.GetDependencyDumpingHandler()
            ->SetDumpAsNormalCompilationOutput();
      } else {
        std::cerr << "glslc: error: both -M (or -MM) and -MD are specified. "
                     "Only one should be used at one time."
                  << std::endl;
        return 1;
      }
    } else if (arg == "-MD") {
      if (compiler.GetDependencyDumpingHandler()->DumpingModeNotSet()) {
        compiler.GetDependencyDumpingHandler()
            ->SetDumpToExtraDependencyInfoFiles();
      } else {
        std::cerr << "glslc: error: both -M (or -MM) and -MD are specified. "
                     "Only one should be used at one time."
                  << std::endl;
        return 1;
      }
    } else if (arg == "-MF") {
      string_piece dep_file_name;
      if (!GetOptionArgument(argc, argv, &i, "-MF", &dep_file_name)) {
        std::cerr
            << "glslc: error: missing dependency info filename after '-MF'"
            << std::endl;
        return 1;
      }
      compiler.GetDependencyDumpingHandler()->SetDependencyFileName(
          std::string(dep_file_name.data(), dep_file_name.size()));
    } else if (arg == "-MT") {
      string_piece dep_file_name;
      if (!GetOptionArgument(argc, argv, &i, "-MT", &dep_file_name)) {
        std::cerr << "glslc: error: missing dependency info target after '-MT'"
                  << std::endl;
        return 1;
      }
      compiler.GetDependencyDumpingHandler()->SetTarget(
          std::string(dep_file_name.data(), dep_file_name.size()));
    } else if (arg == "-S") {
      compiler.SetDisassemblyFlag();
    } else if (arg.starts_with("-D")) {
      const size_t length = arg.size();
      if (length <= 2) {
        std::cerr << "glslc: error: argument to '-D' is missing" << std::endl;
      } else {
        const string_piece argument = arg.substr(2);
        // Get the exact length of the macro string.
        size_t equal_sign_loc = argument.find_first_of('=');
        size_t name_length = equal_sign_loc != shaderc_util::string_piece::npos
                                 ? equal_sign_loc
                                 : argument.size();
        const string_piece name_piece = argument.substr(0, name_length);
        if (name_piece.starts_with("GL_")) {
          std::cerr
              << "glslc: error: names beginning with 'GL_' cannot be defined: "
              << arg << std::endl;
          return 1;
        }
        if (name_piece.find("__") != string_piece::npos) {
          std::cerr
              << "glslc: warning: names containing consecutive underscores "
                 "are reserved: "
              << arg << std::endl;
        }

        const string_piece value_piece =
            (equal_sign_loc == string_piece::npos ||
             equal_sign_loc == argument.size() - 1)
                ? ""
                : argument.substr(name_length + 1);
        // TODO(deki): check arg for newlines.
        compiler.options().AddMacroDefinition(
            name_piece.data(), name_piece.size(), value_piece.data(),
            value_piece.size());
      }
    } else if (arg.starts_with("-I")) {
      string_piece option_arg;
      if (!GetOptionArgument(argc, argv, &i, "-I", &option_arg)) {
        std::cerr
            << "glslc: error: argument to '-I' is missing (expected 1 value)"
            << std::endl;
        success = false;
      } else {
        compiler.AddIncludeDirectory(option_arg.str());
      }
    } else if (arg == "-g") {
      compiler.options().SetGenerateDebugInfo();
    } else if (arg.starts_with("-O")) {
      if (arg == "-Os") {
        compiler.options().SetOptimizationLevel(
            shaderc_optimization_level_size);
      } else if (arg == "-O0") {
        compiler.options().SetOptimizationLevel(
            shaderc_optimization_level_zero);
      } else {
        std::cerr << "glslc: error: invalid value '"
                  << arg.substr(std::strlen("-O")) << "' in '" << arg << "'"
                  << std::endl;
        return 1;
      }
    } else if (arg == "-w") {
      compiler.options().SetSuppressWarnings();
    } else if (arg == "-Werror") {
      compiler.options().SetWarningsAsErrors();
    } else if (!(arg == "-") && arg[0] == '-') {
      std::cerr << "glslc: error: "
                << (arg[1] == '-' ? "unsupported option" : "unknown argument")
                << ": '" << arg << "'" << std::endl;
      return 1;
    } else {
      if (arg == "-") {
        if (has_stdin_input) {
          std::cerr << "glslc: error: specifying standard input \"-\" as input "
                    << "more than once is not allowed." << std::endl;
          return 1;
        }
        has_stdin_input = true;
      }

      const auto language = source_language_forced
                                ? current_source_language
                                : ((glslc::GetFileExtension(arg) == "hlsl")
                                       ? shaderc_source_language_hlsl
                                       : shaderc_source_language_glsl);

      // If current_fshader_stage is shaderc_glsl_infer_from_source, that means
      // we didn't set forced shader kinds (otherwise an error should have
      // already been emitted before). So we should deduce the shader kind
      // from the file name. If current_fshader_stage is specifed to one of
      // the forced shader kinds, use that for the following compilation.
      input_files.emplace_back(glslc::InputFileSpec{
          arg.str(), (current_fshader_stage == shaderc_glsl_infer_from_source
                          ? glslc::DeduceDefaultShaderKindFromFileName(arg)
                          : current_fshader_stage),
          language, current_entry_point_name});
    }
  }

  if (!compiler.ValidateOptions(input_files.size())) return 1;

  if (!success) return 1;

  for (const auto& input_file : input_files) {
    success &= compiler.CompileShaderFile(input_file);
  }

  compiler.OutputMessages();
  return success ? 0 : 1;
}
