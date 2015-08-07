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
#include <string>
#include <utility>

#include "libshaderc_util/string_piece.h"
#include "libshaderc_util/version_profile.h"

#include "file_compiler.h"
#include "file.h"
#include "shader_stage.h"

using shaderc_util::string_piece;

namespace {

// Prints the help message.
void PrintHelp(std::ostream* out) {
  *out << "Usage: glslc [options] file...\n\n"
       << "An input file of - represents standard input.\n\n"
       << "Options:\n"
       << "  -c                Only run preprocess, compile, and assemble"
       << " steps.\n"
       << "  -Dmacro[=defn]    Add an implicit macro definition.\n"
       << "  -E                Outputs only the results of the preprocessing"
       << " step.\n"
       << "                    Output defaults to standard out.\n"
       << "  -fshader-stage=<stage>\n"
       << "                    Treat subsequent input files as having stage "
       << "<stage>.\n"
       << "                    Valid stages are vertex, fragment, tesscontrol, "
       << "tesseval,\n"
       << "                    geometry, and compute.\n"
       << "  -g                Generate source-level debug information.\n"
       << "                    Currently this option has no effect.\n"
       << "  --help            Display available options.\n"
       << "  -I <value>        Add directory to include search path.\n"
       << "  -o <file>         Write output to <file>.\n"
       << "                    A file name of '-' represents standard output.\n"
       << "  -std=<value>      Version and profile for input files. Possible "
       << "values\n"
       << "                    are concatenations of version and profile, e.g. "
       << "310es,\n"
       << "                    450core, etc.\n"
       << "  -S                Only run preprocess and compilation steps.\n"
       << "  -w                Suppresses all warning messages.\n"
       << "  -Werror           Treat all warnings as errors.\n"
       << "  -x <language>     Treat subsequent input files as having type "
       << "<language>.\n"
       << "                    The only supported language is glsl."
       << std::endl;
}

// Parses the argument for the option at index in argv. On success, returns
// true and writes the parsed argument into option_argument. Returns false
// if any error occurs. option is the expected option at argv[index].
// After calling this function, index will be at the last command line
// argument consumed.
bool GetOptionArgument(int argc, char** argv, int* index,
                       const std::string& option,
                       string_piece* option_argument) {
  const string_piece arg = argv[*index];
  assert(arg.starts_with(option));
  if (arg.size() != option.size()) {
    *option_argument = arg.substr(option.size());
    return true;
  } else {
    if (++(*index) >= argc) {
      return false;
    } else {
      *option_argument = argv[*index];
      return true;
    }
  }
}

}  // anonymous namespace

int main(int argc, char** argv) {
  std::vector<std::pair<std::string, EShLanguage>> input_files;
  EShLanguage force_shader_stage = EShLangCount;
  glslc::FileCompiler compiler;
  bool success = true;
  bool has_stdin_input = false;

  compiler.AddIncludeDirectory("");

  for (int i = 1; i < argc; ++i) {
    const string_piece arg = argv[i];
    if (arg == "--help") {
      ::PrintHelp(&std::cout);
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
      force_shader_stage = glslc::GetShaderStageFromCmdLine(arg);
      if (force_shader_stage == EShLangCount) {
        std::cerr << "glslc: error: stage not recognized: '" << stage << "'"
                  << std::endl;
        return 1;
      }
    } else if (arg.starts_with("-std=")) {
      const string_piece standard = arg.substr(std::strlen("-std="));
      int version;
      EProfile profile;
      if (!shaderc_util::ParseVersionProfile(standard.str(), &version,
                                             &profile)) {
        std::cerr << "glslc: error: invalid value '" << standard
                  << "' in '-std=" << standard << "'" << std::endl;
        return 1;
      }
      compiler.SetForcedVersionProfile(version, profile);
    } else if (arg.starts_with("-x")) {
      string_piece option_arg;
      if (!GetOptionArgument(argc, argv, &i, "-x", &option_arg)) {
        std::cerr
            << "glslc: error: argument to '-x' is missing (expected 1 value)"
            << std::endl;
        success = false;
      } else {
        if (option_arg != "glsl") {
          std::cerr << "glslc: error: language not recognized: '" << option_arg
                    << "'" << std::endl;
          return 1;
        }
      }
    } else if (arg == "-c") {
      compiler.SetIndividualCompilationMode();
    } else if (arg == "-E") {
      compiler.SetPreprocessingOnlyMode();
    } else if (arg == "-S") {
      compiler.SetDisassemblyMode();
    } else if (arg.starts_with("-D")) {
      const size_t length = arg.size();
      if (length <= 2) {
        std::cerr << "glslc: error: argument to '-D' is missing" << std::endl;
      } else {
        const string_piece argument = arg.substr(2);
        size_t name_length = argument.find_first_of('=');
        const string_piece name = argument.substr(0, name_length);
        if (name.starts_with("GL_")) {
          std::cerr
              << "glslc: error: names beginning with 'GL_' cannot be defined: "
              << arg << std::endl;
          return 1;
        }
        if (name.find("__") != string_piece::npos) {
          std::cerr
              << "glslc: warning: names containing consecutive underscores "
                 "are reserved: "
              << arg << std::endl;
        }
        // TODO(deki): check arg for newlines.
        compiler.AddMacroDefinition(name, name_length < argument.size()
                                              ? argument.substr(name_length + 1)
                                              : "");
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
      compiler.SetGenerateDebugInfo();
    } else if (arg == "-w") {
      compiler.SetSuppressWarnings();
    } else if (arg == "-Werror") {
      compiler.SetWarningsAsErrors();
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
      input_files.emplace_back(arg.str(), force_shader_stage);
    }
  }

  if (!compiler.ValidateOptions(input_files.size())) return 1;

  if (!success) return 1;

  for (const auto& input_file : input_files) {
    const std::string& name = input_file.first;
    const EShLanguage stage = input_file.second;

    success &= compiler.CompileShaderFile(name, stage);
  }

  compiler.OutputMessages();
  return success ? 0 : 1;
}
