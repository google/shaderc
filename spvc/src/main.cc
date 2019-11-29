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

#include <stdlib.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>
#include <spirv_msl.hpp>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "libshaderc_util/args.h"
#include "libshaderc_util/string_piece.h"
#include "shaderc/env.h"
#include "shaderc/status.h"
#include "spirv-tools/libspirv.h"
#include "spvc/spvc.h"
#include "spvc/spvc.hpp"

#ifdef HAVE_SPVC_GIT_VERSION
#include "gitversion.h"
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

using shaderc_util::string_piece;
using namespace spv;
using namespace SPIRV_CROSS_NAMESPACE;
using namespace std;

namespace {

// Prints the help message.
void PrintHelp(std::ostream *out) {
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

bool ReadFile(const std::string &path, std::vector<uint32_t> *out) {
  FILE *file =
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

struct PLSArg {
  PlsFormat format;
  string name;
};

struct Remap {
  string src_name;
  string dst_name;
  unsigned components;
};

struct VariableTypeRemap {
  string variable_name;
  string new_variable_type;
};

struct InterfaceVariableRename {
  StorageClass storageClass;
  uint32_t location;
  string variable_name;
};

struct CLIArguments {
 public:
  const char *input = nullptr;
  const char *output = nullptr;
  const char *cpp_interface_name = nullptr;
  uint32_t version = 0;
  uint32_t shader_model = 0;
  uint32_t msl_version = 0;
  bool es = false;
  bool set_version = false;
  bool set_shader_model = false;
  bool set_msl_version = false;
  bool set_es = false;
  bool dump_resources = false;
  bool force_temporary = false;
  bool flatten_ubo = false;
  bool fixup = false;
  bool yflip = false;
  bool sso = false;
  bool support_nonzero_baseinstance = true;
  bool msl_capture_output_to_buffer = false;
  bool msl_swizzle_texture_samples = false;
  bool msl_ios = false;
  bool msl_pad_fragment_output = false;
  bool msl_domain_lower_left = false;
  bool msl_argument_buffers = false;
  bool msl_texture_buffer_native = false;
  bool msl_framebuffer_fetch = false;
  bool msl_invariant_float_math = false;
  bool msl_emulate_cube_array = false;
  bool msl_multiview = false;
  bool msl_view_index_from_device_index = false;
  bool msl_dispatch_base = false;
  bool glsl_emit_push_constant_as_ubo = false;
  bool glsl_emit_ubo_as_plain_uniforms = false;
  bool vulkan_glsl_disable_ext_samplerless_texture_functions = false;
  bool emit_line_directives = false;
  spirv_cross::SmallVector<uint32_t> msl_discrete_descriptor_sets;
  spirv_cross::SmallVector<uint32_t> msl_device_argument_buffers;
  spirv_cross::SmallVector<pair<uint32_t, uint32_t>> msl_dynamic_buffers;
  spirv_cross::SmallVector<PLSArg> pls_in;
  spirv_cross::SmallVector<PLSArg> pls_out;
  spirv_cross::SmallVector<Remap> remaps;
  spirv_cross::SmallVector<std::string> extensions;
  spirv_cross::SmallVector<VariableTypeRemap> variable_type_remaps;
  spirv_cross::SmallVector<InterfaceVariableRename> interface_variable_renames;
  spirv_cross::SmallVector<spirv_cross::HLSLVertexAttributeRemap>
      hlsl_attr_remap;
  std::string entry;
  std::string entry_stage;

  struct Rename {
    std::string old_name;
    std::string new_name;
    spv::ExecutionModel execution_model;
  };
  spirv_cross::SmallVector<Rename> entry_point_rename;

  uint32_t iterations = 1;
  bool cpp = false;
  std::string reflect;
  bool msl = false;
  bool hlsl = false;
  bool hlsl_compat = false;
  bool hlsl_support_nonzero_base = false;
  spirv_cross::HLSLBindingFlags hlsl_binding_flags = 0;
  bool vulkan_semantics = false;
  bool flatten_multidimensional_arrays = false;
  bool use_420pack_extension = true;
  bool remove_unused = false;
  bool combined_samplers_inherit_bindings = false;
};

void compile_iteration(const CLIArguments &args,
                       // std::vector<uint32_t> spirv_file,
                       const shaderc_spvc_context_t context) {
  // TBR: args will be used to set compiler flags
}

static void print_help() {
  // print_version();
  fprintf(stderr,
          "Usage: spirv-cross\n"
          "\t[--output <output path>]\n"
          "\t[SPIR-V file]\n"
          "\t[--es]\n"
          "\t[--no-es]\n"
          "\t[--version <GLSL version>]\n"
          "\t[--dump-resources]\n"
          "\t[--help]\n"
          "\t[--revision]\n"
          "\t[--force-temporary]\n"
          "\t[--vulkan-semantics]\n"
          "\t[--flatten-ubo]\n"
          "\t[--fixup-clipspace]\n"
          "\t[--flip-vert-y]\n"
          "\t[--iterations iter]\n"
          "\t[--cpp]\n"
          "\t[--cpp-interface-name <name>]\n"
          "\t[--glsl-emit-push-constant-as-ubo]\n"
          "\t[--glsl-emit-ubo-as-plain-uniforms]\n"
          "\t[--vulkan-glsl-disable-ext-samplerless-texture-functions]\n"
          "\t[--msl]\n"
          "\t[--msl-version <MMmmpp>]\n"
          "\t[--msl-capture-output]\n"
          "\t[--msl-swizzle-texture-samples]\n"
          "\t[--msl-ios]\n"
          "\t[--msl-pad-fragment-output]\n"
          "\t[--msl-domain-lower-left]\n"
          "\t[--msl-argument-buffers]\n"
          "\t[--msl-texture-buffer-native]\n"
          "\t[--msl-framebuffer-fetch]\n"
          "\t[--msl-emulate-cube-array]\n"
          "\t[--msl-discrete-descriptor-set <index>]\n"
          "\t[--msl-device-argument-buffer <index>]\n"
          "\t[--msl-multiview]\n"
          "\t[--msl-view-index-from-device-index]\n"
          "\t[--msl-dispatch-base]\n"
          "\t[--msl-dynamic-buffer <set index> <binding>]\n"
          "\t[--hlsl]\n"
          "\t[--reflect]\n"
          "\t[--shader-model]\n"
          "\t[--hlsl-enable-compat]\n"
          "\t[--hlsl-support-nonzero-basevertex-baseinstance]\n"
          "\t[--hlsl-auto-binding (push, cbv, srv, uav, sampler, all)]\n"
          "\t[--separate-shader-objects]\n"
          "\t[--pls-in format input-name]\n"
          "\t[--pls-out format output-name]\n"
          "\t[--remap source_name target_name components]\n"
          "\t[--extension ext]\n"
          "\t[--entry name]\n"
          "\t[--stage <stage (vert, frag, geom, tesc, tese comp)>]\n"
          "\t[--remove-unused-variables]\n"
          "\t[--flatten-multidimensional-arrays]\n"
          "\t[--no-420pack-extension]\n"
          "\t[--remap-variable-type <variable_name> <new_variable_type>]\n"
          "\t[--rename-interface-variable <in|out> <location> "
          "<new_variable_name>]\n"
          "\t[--set-hlsl-vertex-input-semantic <location> <semantic>]\n"
          "\t[--rename-entry-point <old> <new> <stage>]\n"
          "\t[--combined-samplers-inherit-bindings]\n"
          "\t[--no-support-nonzero-baseinstance]\n"
          "\t[--emit-line-directives]\n"
          "\n");
}

static bool GetNextInt(int argc, char **argv, int *index,
                       const std::string &option, uint32_t *result) {
  string_piece str;
  if (GetOptionArgument(argc, argv, index, option, &str)) {
    shaderc_util::ParseUint32(str.str(), result);
    return true;
  } else {
    return false;
  }
}

static bool GetNextString(int argc, char **argv, int *index,
                          const std::string &option, string_piece *str) {
  if (GetOptionArgument(argc, argv, index, option, str)) {
    return true;
  } else {
    return false;
  }
}

static spirv_cross::HLSLBindingFlags hlsl_resource_type_to_flag(
    const std::string &arg) {
  if (arg == "push")
    return HLSL_BINDING_AUTO_PUSH_CONSTANT_BIT;
  else if (arg == "cbv")
    return HLSL_BINDING_AUTO_CBV_BIT;
  else if (arg == "srv")
    return HLSL_BINDING_AUTO_SRV_BIT;
  else if (arg == "uav")
    return HLSL_BINDING_AUTO_UAV_BIT;
  else if (arg == "sampler")
    return HLSL_BINDING_AUTO_SAMPLER_BIT;
  else if (arg == "all")
    return HLSL_BINDING_AUTO_ALL;
  else {
    fprintf(stderr, "Invalid resource type for --hlsl-auto-binding: %s\n",
            arg.c_str());
    return 0;
  }
}

}  // anonymous namespace

int main(int argc, char **argv) {
  shaderc_spvc::Context context;
  shaderc_spvc::CompileOptions options;
  std::vector<uint32_t> input;
  std::vector<uint32_t> msl_discrete_descriptor;
  string_piece output_path;
  string_piece output_language;
  string_piece source_env = "vulkan1.0";
  string_piece target_env = "";
  CLIArguments cli_args;

  for (int i = 1; i < argc; ++i) {
    const string_piece arg = argv[i];
    // spvc specific
    if (arg == "-v") {
      std::cout << kBuildVersion << std::endl;
      std::cout << "Target: " << spvTargetEnvDescription(SPV_ENV_UNIVERSAL_1_0)
                << std::endl;
      return 0;
    }

    // doing the same as spirv-cross
    if (arg == "--help") {
      // TODO(sarahM0): mix these two
      // edit: -o | --outpout
      // TODO(sarahM0): combine the two help functions
      ::PrintHelp(&std::cout);
      print_help();
    } else if (arg.starts_with("--revision")) {
      // TODO(sarahM0): write print_version and uncomment next line
      // print_version();
      ;
    } else if (arg.starts_with("-o") || arg.starts_with("--output")) {
      if (!shaderc_util::GetOptionArgument(argc, argv, &i, "-o",
                                           &output_path)) {
        std::cerr
            << "spvc: error: argument to '-o' is missing (expected 1 value)"
            << std::endl;
        return 1;
      }
      cli_args.output = output_path.data();
    } else if (arg == "--es") {
      cli_args.es = true;
      cli_args.set_es = true;
      // clang-format off
    } else if (arg == "--no-es") { cli_args.es = false; cli_args.set_es = false;
    } else if (arg == "--version") {cli_args.set_version = true; GetNextInt(argc, argv, &i, "--version", &cli_args.version);
    } else if (arg == "--iterations") { GetNextInt(argc, argv, &i, "--iterations", &cli_args.iterations); //std::cerr<<"iterations: " << &cli_args.iterations;
    } else if (arg.starts_with("--dump-resources")) { cli_args.dump_resources = true;
    } else if (arg == "--force-temporary") { cli_args.force_temporary = true;
    } else if (arg == "--flatten-ubo") { cli_args.flatten_ubo = true;
    } else if (arg == "--fixup-clipspace") { cli_args.fixup = true;
    } else if (arg == "--flip-vert-y") { cli_args.yflip = true;
    } else if (arg == "--cpp") { cli_args.cpp = true;
    } else if (arg == "--reflect") {
      string_piece h;
      std::string default_value = "json";
      bool found = GetNextString(argc, argv, &i, "--reflect", &h);
      cli_args.reflect = h.data();
      if (!found || (strncmp(cli_args.reflect.c_str(), "--", 2) == 0)) cli_args.reflect = default_value;
    } else if (arg == "--cpp-interface-name"){
      string_piece h;
      GetNextString(argc, argv, &i, "--cpp-interface-name", &h);
      cli_args.cpp_interface_name = h.data();
    } else if (arg == "--metal") { cli_args.msl = true;
    } else if (arg == "--glsl-emit-push-constant-as-ubo") { cli_args.glsl_emit_push_constant_as_ubo = true;
    } else if (arg == "--glsl-emit-ubo-as-plain-uniforms") { cli_args.glsl_emit_ubo_as_plain_uniforms = true;
    } else if (arg == "--vulkan-glsl-disable-ext-samplerless-texture-functions") { cli_args.vulkan_glsl_disable_ext_samplerless_texture_functions = true;
    } else if (arg == "--msl") { cli_args.msl = true;
    } else if (arg == "--hlsl") { cli_args.hlsl = true;
    } else if (arg == "--hlsl-enable-compat") { cli_args.hlsl_compat = true;
    } else if (arg == "--hlsl-support-nonzero-basevertex-baseinstance") { cli_args.hlsl_support_nonzero_base = true;
    } else if (arg == "--hlsl-auto-binding") {
      // TODO(sarahM0): make sure this works
      string_piece p;
      GetNextString(argc, argv, &i, "--hlsl-auto-binding", &p);
      cli_args.hlsl_binding_flags |= hlsl_resource_type_to_flag(p.data());
    } else if(arg == "--vulkan-semantics") { cli_args.vulkan_semantics = true;
    } else if(arg == "--flatten-multidimensional-arrays") { cli_args.flatten_multidimensional_arrays = true;
    } else if(arg == "--no-420pack-extension") { cli_args.use_420pack_extension = false;
    } else if(arg == "--msl-capture-output") { cli_args.msl_capture_output_to_buffer = true;
    } else if(arg == "--msl-swizzle-texture-samples") { cli_args.msl_swizzle_texture_samples = true;
    } else if(arg == "--msl-ios") { cli_args.msl_ios = true;
    } else if(arg == "--msl-pad-fragment-output") { cli_args.msl_pad_fragment_output = true;
    } else if(arg == "--msl-domain-lower-left") { cli_args.msl_domain_lower_left = true;
    } else if(arg == "--msl-argument-buffers") { cli_args.msl_argument_buffers = true;
    } else if(arg == "--msl-discrete-descriptor-set") {
      uint32_t p;
      if(GetNextInt(argc, argv, &i, "--msl-discrete-descriptor-set", &p))
        cli_args.msl_discrete_descriptor_sets.push_back(p);
    } else if(arg == "--msl-device-argument-buffer"){
      uint32_t p;
      if(GetNextInt(argc, argv, &i, "--msl-device-argument-buffer", &p))
        cli_args.msl_device_argument_buffers.push_back(p);
    }else if(arg == "--msl-texture-buffer-native") { cli_args.msl_texture_buffer_native = true;
    }else if(arg == "--msl-framebuffer-fetch") { cli_args.msl_framebuffer_fetch = true;
    }else if(arg == "--msl-invariant-float-math") { cli_args.msl_invariant_float_math = true;
    }else if(arg == "--msl-emulate-cube-array") { cli_args.msl_emulate_cube_array = true;
    }else if(arg == "--msl-multiview") { cli_args.msl_multiview = true;
    }else if(arg == "--msl-view-index-from-device-index") { cli_args.msl_view_index_from_device_index = true;
    }else if(arg == "--msl-dispatch-base") { cli_args.msl_dispatch_base = true;
    }else if(arg == "--msl-dynamic-buffer") {
    cli_args.msl_argument_buffers = true;
    //spirv-cross comment: Make sure next_uint() is called in-order.
    uint32_t desc_set;
    GetNextInt(argc, argv, &i, "--msl-dynamic-buffer", &desc_set);
    uint32_t binding = 0;
    // TODO(sarahM0): figure out how to read the second int - uncomment next line
    // GetNextInt(argc, argv, &i, option , &binding);
    cli_args.msl_dynamic_buffers.push_back(make_pair(desc_set, binding));
      // clang-format on
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
    } else if (arg == "--no-validate") {
      options.SetValidate(false);
    } else if (arg == "--no-optimize") {
      options.SetOptimize(false);
    } else if (arg == "--robust-buffer-access-pass") {
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
  shaderc_compilation_status status =
      shaderc_compilation_status_configuration_error;

  if (output_language == "glsl") {
    status = context.InitializeForGlsl((const uint32_t *)input.data(),
                                       input.size(), options);
  } else if (output_language == "msl") {
    status = context.InitializeForMsl((const uint32_t *)input.data(),
                                      input.size(), options);
  } else if (output_language == "hlsl") {
    status = context.InitializeForHlsl((const uint32_t *)input.data(),
                                       input.size(), options);
  } else if (output_language == "vulkan") {
    status = context.InitializeForVulkan((const uint32_t *)input.data(),
                                         input.size(), options);
  } else {
    std::cerr << "Attempted to output to unknown language: " << output_language
              << std::endl;
    return 1;
  }

  compile_iteration(cli_args, reinterpret_cast<shaderc_spvc_context_t>(
                                  context.GetCompiler()));

  std::cerr << status << "\n";
  if (status == shaderc_compilation_status_success)
    status = context.CompileShader(&result);
  std::cerr << status << "\n";
  switch (status) {
    case shaderc_compilation_status_validation_error: {
      std::cerr << "validation failed:\n" << context.GetMessages() << std::endl;
      return 1;
    }

    case shaderc_compilation_status_success: {
      const char *path = output_path.data();
      if (output_language != "vulkan") {
        if (path && strcmp(path, "-")) {
          std::basic_ofstream<char>(path) << result.GetStringOutput();
        } else {
          std::cout << result.GetStringOutput();
        }
      } else {
        if (path && strcmp(path, "-")) {
          std::ofstream out(path, std::ios::binary);
          out.write((char *)result.GetBinaryOutput().data(),
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

    case shaderc_compilation_status_compilation_error: {
      std::cerr << "compilation failed:\n"
                << context.GetMessages() << std::endl;
      return 1;
    }

    default:
      std::cerr << "unexpected error " << status << std::endl;
      return 1;
  }
}
