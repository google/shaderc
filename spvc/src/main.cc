// Copyright 2019 The Shaderc Authors. All rights reserved.
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

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "libshaderc_util/string_piece.h"
#include "shaderc/env.h"
#include "shaderc/spvc.hpp"
#include "shaderc/status.h"
#include "spirv-tools/libspirv.h"

using shaderc_util::string_piece;

namespace {

// Prints the help message.
void PrintHelp(std::ostream* out) {
  *out << R"(spvc - Compile SPIR-V into GLSL/HLSL/MSL.

Usage: spvc [options] file

An input file of - represents standard input.

Options:
  --help                Display available options.
  --version             Display compiler version information.
  -o <output file>      '-' means standard output.
  --validate=[<env>]    Validate SPIR-V source with given environment
                        <env> is empty to skip validation, or one of the following:
                        vulkan1.0 (default) vulkan1.1 opengl opengl4.5
)";
}

// TODO(fjhenigman): factor out
// Gets the option argument for the option at *index in argv in a way consistent
// with clang/gcc. On success, returns true and writes the parsed argument into
// *option_argument. Returns false if any errors occur. After calling this
// function, *index will be the index of the last command line argument
// consumed.
bool GetOptionArgument(int argc, char** argv, int* index,
                       const std::string& option,
                       string_piece* option_argument) {
  const string_piece arg = argv[*index];
  assert(arg.starts_with(option));
  if (arg.size() != option.size()) {
    *option_argument = arg.substr(option.size());
    return true;
  }
  if (option.back() == '=') {
    *option_argument = "";
    return true;
  }
  if (++(*index) >= argc)
    return false;
  *option_argument = argv[*index];
  return true;

}

const char kBuildVersion[] = ""
    // TODO(fjhenigman): #include "build-version.inc"
    ;

bool ReadFile(const std::string& path, std::vector<uint32_t>* out) {
  FILE* file =
      path == "-" ? freopen(nullptr, "rb", stdin) : fopen(path.c_str(), "rb");
  if (!file) {
    std::cerr << "Failed to open SPIR-V file: " << path << std::endl;
    return false;
  }

  fseek(file, 0, SEEK_END);
  out->resize(ftell(file) / sizeof((*out)[0]));
  rewind(file);

  if (fread(out->data(), sizeof((*out)[0]), out->size(), file) != out->size()) {
    std::cerr << "Failed to read SPIR-V file: " << path << std::endl;
    out->clear();
    return false;
  }

  fclose(file);
  return true;
}

}  // anonymous namespace

int main(int argc, char** argv) {
  shaderc_spvc::Compiler compiler;
  shaderc_spvc::CompileOptions options;
  std::vector<uint32_t> input;
  string_piece output_path;

  for (int i = 1; i < argc; ++i) {
    const string_piece arg = argv[i];
    if (arg == "--help") {
      ::PrintHelp(&std::cout);
      return 0;
    } else if (arg == "--version") {
      std::cout << kBuildVersion << std::endl;
      std::cout << "Target: " << spvTargetEnvDescription(SPV_ENV_UNIVERSAL_1_0)
                << std::endl;
      return 0;
    } else if (arg.starts_with("-o")) {
      if (!GetOptionArgument(argc, argv, &i, "-o", &output_path)) {
        std::cerr
            << "spvc: error: argument to '-o' is missing (expected 1 value)"
            << std::endl;
        return 1;
      }
    } else if (arg == "--remove-unused-variables") {
      // TODO
    } else if (arg.starts_with("--validate=")) {
      string_piece target_env;
      GetOptionArgument(argc, argv, &i, "--validate=", &target_env);
      if (target_env == "vulkan") {
        options.SetTargetEnvironment(shaderc_target_env_vulkan,
                                     shaderc_env_version_vulkan_1_0);
      } else if (target_env == "vulkan1.0") {
        options.SetTargetEnvironment(shaderc_target_env_vulkan,
                                     shaderc_env_version_vulkan_1_0);
      } else if (target_env == "vulkan1.1") {
        options.SetTargetEnvironment(shaderc_target_env_vulkan,
                                     shaderc_env_version_vulkan_1_1);
      } else {
        std::cerr << "spvc: error: invalid value '" << target_env
                  << "' in '--validate=" << target_env << "'" << std::endl;
        return 1;
      }
    } else {
      if (!ReadFile(arg.str(), &input)) {
        std::cerr << "spvc: error: could not read file" << std::endl;
        return 1;
      }
    }
  }

  shaderc_spvc::CompilationResult result = compiler.CompileSpvToGlsl(
      (const uint32_t*)input.data(), input.size(), options);
  auto status = result.GetCompilationStatus();
  if (status == shaderc_compilation_status_validation_error) {
    std::cerr << "validation failed:\n" << result.GetMessages() << std::endl;
    return 1;
  }
  if (status == shaderc_compilation_status_success) {
    const char* path = output_path.data();
    if (path && strcmp(path, "-")) {
      std::basic_ofstream<char>(path) << result.GetOutput();
    } else {
      std::cout << result.GetOutput();
    }
    return 0;
  }
  if (status == shaderc_compilation_status_compilation_error) {
    std::cerr << "compilation failed:\n" << result.GetMessages() << std::endl;
    return 1;
  }

  std::cerr << "unexpected error " << status << std::endl;
  return 1;
}
