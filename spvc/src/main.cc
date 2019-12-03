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

#include "libshaderc_util/args.h"
#include "libshaderc_util/string_piece.h"
#include "shaderc/env.h"
#include "spvc/spvc.hpp"
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
  --help                   Display available options.
  -v                       Display compiler version information.
  -o <output file>         '-' means standard output.
  --no-validate            Disable validating input and intermediate source.
                             Validation is by default enabled.
  --no-optimize            Disable optimizing input and intermediate source.
                             Optimization is by default enabled.
  --source-env=<env>       Execution environment of the input source.
                             <env> is vulkan1.0 (the default), vulkan1.1,
                             or webgpu0
  --entry=<name>           Specify entry point.
  --language=<lang>        Specify output language.
                             <lang> is glsl (the default), msl or hlsl.
  --glsl-version=<ver>     Specify GLSL output language version, e.g. 100
                             Default is 450 if not detected from input.
  --msl-version=<ver>      Specify MSL output language version, e.g. 100
                             Default is 10200.
  --target-env=<env>       Target intermediate execution environment to
                           transform the source to before cross-compiling.
                           Defaults to the value set for source-env.
                           <env> must be one of the legal values for source-env.

                           If target-env and source-env are the same, then no
                           transformation is performed.
                           If there is no defined transformation between source
                           and target the operation will fail.
                           Defined transforms:
                             webgpu0 -> vulkan1.1
                             vulkan1.1 -> webgpu0
   --use-spvc-parser       Use the built in parser for generating spirv-cross IR
                           instead of spirv-cross.


  The following flags behave as in spirv-cross:

  --remove-unused-variables
  --vulkan-semantics
  --separate-shader-objects
  --flatten-ubo
  --flatten-multidimensional-arrays
  --es
  --no-es
  --glsl-emit-push-constant-as-ubo
  --msl-swizzle-texture-samples
  --msl-platform=ios|macos
  --msl-pad-fragment-output
  --msl-capture-output
  --msl-domain-lower-left
  --msl-argument-buffers
  --msl-discrete-descriptor-set=<number>
  --emit-line-directives
  --hlsl-enable-compat
  --shader-model=<model>
)";
}

const char kBuildVersion[] = ""
    // TODO(bug 653): #include "build-version.inc"
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
  shaderc_spvc::Context context;
  shaderc_spvc::CompileOptions options;
  std::vector<uint32_t> input;
  std::vector<uint32_t> msl_discrete_descriptor;
  string_piece output_path;
  string_piece output_language;
  string_piece source_env = "vulkan1.0";
  string_piece target_env = "";

  for (int i = 1; i < argc; ++i) {
    const string_piece arg = argv[i];
    if (arg == "--help") {
      ::PrintHelp(&std::cout);
      return 0;
    } else if (arg == "-v") {
      std::cout << kBuildVersion << std::endl;
      std::cout << "Target: " << spvTargetEnvDescription(SPV_ENV_UNIVERSAL_1_0)
                << std::endl;
      return 0;
    } else if (arg.starts_with("-o")) {
      if (!shaderc_util::GetOptionArgument(argc, argv, &i, "-o",
                                           &output_path)) {
        std::cerr
            << "spvc: error: argument to '-o' is missing (expected 1 value)"
            << std::endl;
        return 1;
      }
    } else if (arg.starts_with("--entry=")) {
      string_piece entry_point;
      shaderc_util::GetOptionArgument(argc, argv, &i, "--entry=", &entry_point);
      options.SetEntryPoint(entry_point.data());
    } else if (arg.starts_with("--glsl-version=")) {
      string_piece version_str;
      shaderc_util::GetOptionArgument(argc, argv, &i,
                                      "--glsl-version=", &version_str);
      uint32_t version_num;
      if (!shaderc_util::ParseUint32(version_str.str(), &version_num)) {
        std::cerr << "spvc: error: invalid value '" << version_str
                  << "' in --glsl-version=" << std::endl;
        return 1;
      }
      options.SetGLSLLanguageVersion(version_num);
    } else if (arg.starts_with("--msl-version=")) {
      string_piece version_str;
      shaderc_util::GetOptionArgument(argc, argv, &i,
                                      "--msl-version=", &version_str);
      uint32_t version_num;
      if (!shaderc_util::ParseUint32(version_str.str(), &version_num)) {
        std::cerr << "spvc: error: invalid value '" << version_str
                  << "' in --msl-version=" << std::endl;
        return 1;
      }
      options.SetMSLLanguageVersion(version_num);
    } else if (arg.starts_with("--language=")) {
      shaderc_util::GetOptionArgument(argc, argv, &i,
                                      "--language=", &output_language);
      if (!(output_language == "glsl" || output_language == "msl" ||
            output_language == "hlsl" || output_language == "vulkan")) {
        std::cerr << "spvc: error: invalid value '" << output_language
                  << "' in --language=" << std::endl;
        return 1;
      }
    } else if (arg == "--remove-unused-variables") {
      options.SetRemoveUnusedVariables(true);
    } else if (arg == "--no-validate"){
      options.SetValidate(false);
    } else if (arg == "--no-optimize"){
      options.SetOptimize(false);
    } else if (arg == "--robust-buffer-access-pass"){
      options.SetRobustBufferAccessPass(true);
    } else if (arg == "--vulkan-semantics") {
      options.SetVulkanSemantics(true);
    } else if (arg == "--separate-shader-objects") {
      options.SetSeparateShaderObjects(true);
    } else if (arg == "--flatten-ubo") {
      options.SetFlattenUbo(true);
    } else if (arg == "--flatten-multidimensional-arrays") {
      options.SetFlattenMultidimensionalArrays(true);
    } else if (arg == "--es") {
      options.SetES(true);
    } else if (arg == "--no-es") {
      options.SetES(false);
    } else if (arg == "--hlsl-enable-compat") {
      options.SetHLSLPointSizeCompat(true);
      options.SetHLSLPointCoordCompat(true);
    } else if (arg == "--glsl-emit-push-constant-as-ubo") {
      options.SetGLSLEmitPushConstantAsUBO(true);
    } else if (arg == "--msl-swizzle-texture-samples") {
      options.SetMSLSwizzleTextureSamples(true);
    } else if (arg.starts_with("--msl-platform=")) {
      string_piece platform;
      GetOptionArgument(argc, argv, &i, "--msl-platform=", &platform);
      if (platform == "ios") {
        options.SetMSLPlatform(shaderc_spvc_msl_platform_ios);
      } else if (platform == "macos") {
        options.SetMSLPlatform(shaderc_spvc_msl_platform_macos);
      } else {
        std::cerr << "spvc: error: invalid value '" << platform
                  << "' in --msl-platform=" << std::endl;
        return 1;
      }
    } else if (arg == "--msl-pad-fragment-output") {
      options.SetMSLPadFragmentOutput(true);
    } else if (arg == "--msl-capture-output") {
      options.SetMSLCapture(true);
    } else if (arg == "--msl-domain-lower-left") {
      options.SetMSLDomainLowerLeft(true);
    } else if (arg == "--msl-argument-buffers") {
      options.SetMSLArgumentBuffers(true);
    } else if (arg.starts_with("--msl-discrete-descriptor-set=")) {
      string_piece descriptor_str;
      GetOptionArgument(argc, argv, &i,
                        "--msl-discrete-descriptor-set=", &descriptor_str);
      uint32_t descriptor_num;
      if (!shaderc_util::ParseUint32(descriptor_str.str(), &descriptor_num)) {
        std::cerr << "spvc: error: invalid value '" << descriptor_str
                  << "' in --msl-discrete-descriptor-set=" << std::endl;
        return 1;
      }
      msl_discrete_descriptor.push_back(descriptor_num);
    } else if (arg == "--emit-line-directives") {
      options.SetEmitLineDirectives(true);
    } else if (arg.starts_with("--shader-model=")) {
      string_piece shader_model_str;
      shaderc_util::GetOptionArgument(argc, argv, &i,
                                      "--shader-model=", &shader_model_str);
      uint32_t shader_model_num;
      if (!shaderc_util::ParseUint32(shader_model_str.str(),
                                     &shader_model_num)) {
        std::cerr << "spvc: error: invalid value '" << shader_model_str
                  << "' in --shader-model=" << std::endl;
        return 1;
      }
      options.SetHLSLShaderModel(shader_model_num);
    } else if (arg.starts_with("--source-env=")) {
      string_piece env;
      shaderc_util::GetOptionArgument(argc, argv, &i, "--source-env=", &env);
      source_env = env;
      if (env == "vulkan1.0") {
        options.SetSourceEnvironment(shaderc_target_env_vulkan,
                                     shaderc_env_version_vulkan_1_0);
      } else if (env == "vulkan1.1") {
        options.SetSourceEnvironment(shaderc_target_env_vulkan,
                                     shaderc_env_version_vulkan_1_1);
      } else if (env == "webgpu0") {
        options.SetSourceEnvironment(shaderc_target_env_webgpu,
                                     shaderc_env_version_webgpu);
      } else {
        std::cerr << "spvc: error: invalid value '" << env
                  << "' in --source-env=" << std::endl;
        return 1;
      }
    } else if (arg.starts_with("--target-env=")) {
      string_piece env;
      shaderc_util::GetOptionArgument(argc, argv, &i, "--target-env=", &env);
      target_env = env;
      if (env == "vulkan1.0") {
        options.SetTargetEnvironment(shaderc_target_env_vulkan,
                                     shaderc_env_version_vulkan_1_0);
      } else if (env == "vulkan1.1") {
        options.SetTargetEnvironment(shaderc_target_env_vulkan,
                                     shaderc_env_version_vulkan_1_1);
      } else if (env == "webgpu0") {
        options.SetTargetEnvironment(shaderc_target_env_webgpu,
                                     shaderc_env_version_webgpu);
      } else {
        std::cerr << "spvc: error: invalid value '" << env
                  << "' in --target-env=" << std::endl;
        return 1;
      }
    } else if (arg == "--use-spvc-parser") {
      context.SetUseSpvcParser(true);
    } else {
      if (!ReadFile(arg.str(), &input)) {
        std::cerr << "spvc: error: could not read file" << std::endl;
        return 1;
      }
    }
  }

  if (target_env == "") {
    if (source_env == "vulkan1.0") {
      options.SetTargetEnvironment(shaderc_target_env_vulkan,
                                   shaderc_env_version_vulkan_1_0);
    } else if (source_env == "vulkan1.1") {
      options.SetTargetEnvironment(shaderc_target_env_vulkan,
                                   shaderc_env_version_vulkan_1_1);
    } else if (source_env == "webgpu0") {
      options.SetTargetEnvironment(shaderc_target_env_webgpu,
                                   shaderc_env_version_webgpu);
    } else {
      // This should be caught above when parsing --source-target=
      std::cerr << "spvc: error: invalid value '" << source_env
                << "' in --source-env=" << std::endl;
      return 1;
    }
  }

  options.SetMSLDiscreteDescriptorSets(msl_discrete_descriptor);

  shaderc_spvc::CompilationResult result;
  shaderc_spvc_status status = shaderc_spvc_status_configuration_error;

  if (output_language == "glsl") {
    status = context.InitializeForGlsl((const uint32_t*)input.data(),
                                       input.size(), options);
  } else if (output_language == "msl") {
    status = context.InitializeForMsl((const uint32_t*)input.data(),
                                      input.size(), options);
  } else if (output_language == "hlsl") {
    status = context.InitializeForHlsl((const uint32_t*)input.data(),
                                       input.size(), options);
  } else if (output_language == "vulkan") {
    status = context.InitializeForVulkan((const uint32_t*)input.data(),
                                         input.size(), options);
  } else {
    std::cerr << "Attempted to output to unknown language: " << output_language
              << std::endl;
    return 1;
  }

  if (status == shaderc_spvc_status_success)
    status = context.CompileShader(&result);

  switch (status) {
    case shaderc_spvc_status_validation_error: {
      std::cerr << "validation failed:\n" << context.GetMessages() << std::endl;
      return 1;
    }

    case shaderc_spvc_status_success: {
      const char* path = output_path.data();
      if (output_language != "vulkan") {
        if (path && strcmp(path, "-")) {
          std::basic_ofstream<char>(path) << result.GetStringOutput();
        } else {
          std::cout << result.GetStringOutput();
        }
      } else {
        if (path && strcmp(path, "-")) {
          std::ofstream out(path, std::ios::binary);
          out.write((char*)result.GetBinaryOutput().data(),
                    (sizeof(uint32_t) / sizeof(char)) *
                        result.GetBinaryOutput().size());
        } else {
          std::cerr << "Cowardly refusing to output binary result to screen"
                    << std::endl;
          return 1;
        }
      }
      return 0;
    }

    case shaderc_spvc_status_compilation_error: {
      std::cerr << "compilation failed:\n"
                << context.GetMessages() << std::endl;
      return 1;
    }

    default:
      std::cerr << "unexpected error " << status << std::endl;
      return 1;
  }
}
