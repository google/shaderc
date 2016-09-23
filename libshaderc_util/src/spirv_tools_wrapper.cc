// Copyright 2016 The Shaderc Authors. All rights reserved.
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

#include "libshaderc_util/spirv_tools_wrapper.h"

#include <algorithm>
#include <cassert>
#include <sstream>

#include "spirv-tools/optimizer.hpp"

namespace {

// Writes the message contained in the diagnostic parameter to *dest. Assumes
// the diagnostic message is reported for a binary location.
void SpirvToolsOutputDiagnostic(spv_diagnostic diagnostic, std::string* dest) {
  std::ostringstream os;
  if (diagnostic->isTextSource) {
    os << diagnostic->position.line + 1 << ":"
       << diagnostic->position.column + 1;
  } else {
    os << diagnostic->position.index;
  }
  os << ": " << diagnostic->error;
  *dest = os.str();
}

}  // anonymous namespace

namespace shaderc_util {

bool SpirvToolsDisassemble(const std::vector<uint32_t>& binary,
                           std::string* text_or_error) {
  auto spvtools_context = spvContextCreate(SPV_ENV_VULKAN_1_0);
  spv_text disassembled_text = nullptr;
  spv_diagnostic spvtools_diagnostic = nullptr;

  const bool result =
      spvBinaryToText(spvtools_context, binary.data(), binary.size(),
                      (SPV_BINARY_TO_TEXT_OPTION_INDENT |
                       SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES),
                      &disassembled_text, &spvtools_diagnostic) == SPV_SUCCESS;
  if (result) {
    text_or_error->assign(disassembled_text->str, disassembled_text->length);
  } else {
    SpirvToolsOutputDiagnostic(spvtools_diagnostic, text_or_error);
  }

  spvDiagnosticDestroy(spvtools_diagnostic);
  spvTextDestroy(disassembled_text);
  spvContextDestroy(spvtools_context);

  return result;
}

bool SpirvToolsAssemble(const string_piece assembly, spv_binary* binary,
                        std::string* errors) {
  auto spvtools_context = spvContextCreate(SPV_ENV_VULKAN_1_0);
  spv_diagnostic spvtools_diagnostic = nullptr;

  *binary = nullptr;
  errors->clear();

  const bool result =
      spvTextToBinary(spvtools_context, assembly.data(), assembly.size(),
                      binary, &spvtools_diagnostic) == SPV_SUCCESS;
  if (!result) SpirvToolsOutputDiagnostic(spvtools_diagnostic, errors);

  spvDiagnosticDestroy(spvtools_diagnostic);
  spvContextDestroy(spvtools_context);

  return result;
}

bool SpirvToolsOptimize(const std::vector<PassId>& enabled_passes,
                        std::vector<uint32_t>* binary, std::string* errors) {
  errors->clear();
  if (enabled_passes.empty()) return true;
  if (std::all_of(
          enabled_passes.cbegin(), enabled_passes.cend(),
          [](const PassId& pass) { return pass == PassId::kNullPass; })) {
    return true;
  }

  spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_0);
  std::ostringstream oss;
  optimizer.SetMessageConsumer(
      [&oss](spv_message_level_t, const char*, const spv_position_t&,
             const char* message) { oss << message << "\n"; });

  for (const auto& pass : enabled_passes) {
    switch (pass) {
      case PassId::kNullPass:
        // We actually don't need to do anything for null pass.
        break;
      case PassId::kStripDebugInfo:
        optimizer.RegisterPass(spvtools::CreateStripDebugInfoPass());
        break;
      case PassId::kUnifyConstant:
        optimizer.RegisterPass(spvtools::CreateUnifyConstantPass());
        break;
    }
  }

  if (!optimizer.Run(binary->data(), binary->size(), binary)) {
    *errors = oss.str();
    return false;
  }
  return true;
}

}  // namespace shaderc_util
