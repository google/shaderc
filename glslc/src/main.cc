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
#include <list>

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
       << "  -working-directory <dir>\n"
       << "                    Resolve file paths relative to the specified "
       << "directory.\n"
       << "  -x <language>     Treat subsequent input files as having type "
       << "<language>.\n"
       << "                    The only supported language is glsl."
       << std::endl;
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

}  // anonymous namespace

int main(int argc, char** argv) {
  std::vector<std::pair<std::string, shaderc_shader_stage>> input_files;
  shaderc_shader_stage current_fshader_stage = shaderc_glsl_infer_from_source;
  glslc::FileCompiler compiler;
  bool success = true;
  bool has_stdin_input = false;
  // Contains all the macro names data. Current libshaderc interface requires
  // null-terminated string, we need to 'pull out' and copy the macro names and
  // hold their data here. Besides, as the internal unordered_map in
  // libshaderc_util is storing string_piece, we need to keep the addresses of
  // our macros and values unchanged. std::list meets this requirement.
  std::list<std::string> macro_names;
  std::list<std::string> macro_values;

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
    } else if (arg.starts_with("-working-directory")) {
      // Following Clang, both -working-directory and -working-directory= are
      // accepted.
      std::string option = "-working-directory";
      if (arg[option.size()] == '=') option = "-working-directory=";
      string_piece workdir;
      if (!GetOptionArgument(argc, argv, &i, option, &workdir)) {
        std::cerr << "glslc: error: argument to '-working-directory' is "
                     "missing (expected 1 value)"
                  << std::endl;
      }
      compiler.SetWorkingDirectory(workdir.str());
    } else if (arg.starts_with("-fshader-stage=")) {
      const string_piece stage = arg.substr(std::strlen("-fshader-stage="));
      current_fshader_stage = glslc::GetForcedShaderStageFromCmdLine(arg);
      if (current_fshader_stage == shaderc_glsl_infer_from_source) {
        std::cerr << "glslc: error: stage not recognized: '" << stage << "'"
                  << std::endl;
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
      compiler.SetIndividualCompilationFlag();
    } else if (arg == "-E") {
      compiler.options().SetPreprocessingOnlyMode();
      compiler.SetPreprocessingOnlyFlag();
    } else if (arg == "-S") {
      compiler.options().SetDisassemblyMode();
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

        // We have to insert a null terminator at the end of name, as we are
        // using libshaderc's API instead of libshaderc_util's. Libshaderc API
        // assumes a const char* string, while libshaderc_util assume a
        // string_piece one. The original implementation doesn't append '\0' but
        // works fine with libshaderc_util because it has 'end_' property. But
        // libshaderc api doesn't know that so the macro 'name' will include the
        // 'value' if we don't insert '\0'.

        macro_names.emplace_back(name_piece.data(), name_length);
        macro_values.emplace_back(
            equal_sign_loc == shaderc_util::string_piece::npos ||
                    equal_sign_loc == argument.size() - 1
                ? ""
                : argument.substr(name_length + 1).data());
        // TODO(deki): check arg for newlines.
        compiler.options().AddMacroDefinition(macro_names.back().c_str(),
                                              macro_values.back().c_str());
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

      // If current_fshader_stage is shaderc_glsl_infer_from_source, that means
      // we didn't set forced shader stages (otherwise an error should have
      // already been emitted before). So we should deduce the shader stage
      // from the file name. If current_fshader_stage is specifed to one of
      // the forced shader stages, use that for the following compilation.
      input_files.emplace_back(
          arg.str(), current_fshader_stage == shaderc_glsl_infer_from_source
                         ? glslc::DeduceDefaultShaderStageFromFileName(arg)
                         : current_fshader_stage );
    }
  }

  if (!compiler.ValidateOptions(input_files.size())) return 1;

  if (!success) return 1;

  for (const auto& input_file : input_files) {
    const std::string& name = input_file.first;
    const shaderc_shader_stage stage = input_file.second;

    success &= compiler.CompileShaderFile(name, stage);
  }

  compiler.OutputMessages();
  return success ? 0 : 1;
}
