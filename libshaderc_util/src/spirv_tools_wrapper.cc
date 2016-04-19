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

#include <cassert>
#include <sstream>

namespace {

// Writes the message contained in the diagnostic parameter to *dest. Assumes
// the diagnostic message is reported for a binary location.
void OutputSpvToolsDiagnostic(spv_diagnostic diagnostic, std::string* dest) {
  assert(!diagnostic->isTextSource);

  std::ostringstream os;
  os << diagnostic->position.index << ": " << diagnostic->error;
  *dest = os.str();
}

}  // anonymous namespace

namespace shaderc_util {

spv_result_t DisassembleBinary(const std::vector<uint32_t>& binary,
                               std::string* text_or_error) {
  auto spvtools_context = spvContextCreate(SPV_ENV_UNIVERSAL_1_0);
  spv_text disassembled_text = nullptr;
  spv_diagnostic spvtools_diagnostic = nullptr;

  spv_result_t result =
      spvBinaryToText(spvtools_context, binary.data(), binary.size(),
                      SPV_BINARY_TO_TEXT_OPTION_INDENT, &disassembled_text,
                      &spvtools_diagnostic);
  if (result == SPV_SUCCESS) {
    text_or_error->assign(disassembled_text->str, disassembled_text->length);
  } else {
    OutputSpvToolsDiagnostic(spvtools_diagnostic, text_or_error);
  }

  spvDiagnosticDestroy(spvtools_diagnostic);
  spvTextDestroy(disassembled_text);
  spvContextDestroy(spvtools_context);

  return result;
}

}  // namespace shaderc_util
