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

#ifndef LIBSHADERC_SPVC_SRC_SPVCIR_PASS_H_
#define LIBSHADERC_SPVC_SRC_SPVCIR_PASS_H_

#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>
#include <spirv_msl.hpp>

#include "source/opt/pass.h"  // Accessing a private spirv-tools header.
#include "spirv-tools/libspirv.hpp"
#include "spvc/spvc.h"

namespace spvtools {
namespace opt {
// this WIP pass generates spvc IR and does not throw exceptions
class SpvcIrPass : public Pass {
 public:
  SpvcIrPass(spirv_cross::ParsedIR *ir);
  const char *name() const override { return "spvc-if-pass"; }
  Status Process() override;

  IRContext::Analysis GetPreservedAnalyses() override {
    return IRContext::kAnalysisNone;
  }

  // Given an spirv Instruction adds its SPIRV-Corss IR equivalent to ir_
  void GenerateSpirvCrossIR(Instruction *inst);

 private:
  spirv_cross::ParsedIR *ir_;
  spirv_cross::SPIRFunction *current_function_ = nullptr;
  spirv_cross::SPIRBlock *current_block_ = nullptr;
  Pass::Status status_;
  // A data structure to store type aliases.
  // spirv-cross comment: This must be an ordered data structure so we always
  // pick the same type aliases.
  spirv_cross::SmallVector<uint32_t> global_struct_cache_;
  // used to save our offset into the input spirv string
  uint32_t offset_ = 5;

  // copied from spirv-cross (class Parser)
  // Adds an instruction to the SPIRV-Cross IR, for the given result Id and with
  // given type and arguments.
  // Returns the constructed instruction.
  // TODO(sarahM0): explore whether it's better to replace with explicit set,
  // se1, set2, ...
  template <typename T, typename... P>
  T &set(uint32_t id, P &&... args) {
    ir_->add_typed_id(static_cast<spirv_cross::Types>(T::type), id);
    auto &var =
        spirv_cross::variant_set<T>(ir_->ids[id], std::forward<P>(args)...);
    var.self = id;
    return var;
  }

  // copied from spirv-cross (class Parser)
  // For a given result Id returns the previously constructed instruction.
  template <typename T>
  T &get(uint32_t id) {
    return spirv_cross::variant_get<T>(ir_->ids[id]);
  }
  template <typename T>
  const T &get(uint32_t id) const {
    return spirv_cross::variant_get<T>(ir_->ids[id]);
  }

  // copied from spirv-cross (class Parser)
  // For a given result Id returns the instruction if it was previously
  // constructed and had the same result Type otherwise returns nullptr.
  template <typename T>
  T *maybe_get(uint32_t id) {
    if (id >= ir_->ids.size())
      return nullptr;
    else if (ir_->ids[id].get_type() ==
             static_cast<spirv_cross::Types>(T::type))
      return &get<T>(id);
    else
      return nullptr;
  }

  template <typename T>
  const T *maybe_get(uint32_t id) const {
    if (id >= ir_->ids.size())
      return nullptr;
    else if (ir_->ids[id].get_type() ==
             static_cast<spirv_cross::Types>(T::type))
      return &get<T>(id);
    else
      return nullptr;
  }

  // Check boolean condition. If false record the error message, and set pass
  // status to failure to stop processing.
  bool CheckConditionAndSetErrorMessage(bool condition, std::string message);

  // Returns true only if to the given spirv-cross types are equivalent
  bool types_are_logically_equivalent(const spirv_cross::SPIRType &a,
                                      const spirv_cross::SPIRType &b) const;

  // Given an id and its type, sets all of its entries and sub-entries to null
  void make_constant_null(uint32_t id, uint32_t type);
};

}  // namespace opt
}  // namespace spvtools

#endif  // LIBSHADERC_SPVC_SRC_SPVCIR_PASS_H_
