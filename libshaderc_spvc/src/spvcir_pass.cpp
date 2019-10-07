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

#include "spvcir_pass.h"

#include <fstream>

namespace spvtools {
namespace opt {

Pass::Status SpvcIrPass::Process() {
  // A temporary pass.
  // TODO (sarahM0): Replace this by a switch-case that generates a spirv-cross
  // IR
  uint32_t options = 0;
  std::ostringstream oss;
  get_module()->ForEachInst([&oss, options](const Instruction* inst) {
    oss << inst->PrettyPrint(options);
    if (!IsTerminatorInst(inst->opcode())) {
      oss << std::endl;
    }
  });
  return Status::SuccessWithoutChange;
}

}  // namespace opt
}  // namespace spvtools
