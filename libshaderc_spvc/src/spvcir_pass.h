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

#include "spvc/spvc.h"
#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>
#include <spirv_msl.hpp>
#include "spirv-tools/libspirv.hpp"
#include "spirv-tools/source/opt/pass.h"


namespace spvtools {
namespace opt {
 // this WIP pass generates spvc IR and does not throw exceptions
class SpvcIrPass : public Pass {
 public:
  SpvcIrPass(spirv_cross::ParsedIR &ir){ ir = ir; }
  const char* name() const override { return "spvc-IR-pass"; }
  void doNothing();
  Status Process() override;
  private:
    spirv_cross::ParsedIR ir;
};

}  // namespace opt
}  // namespace spvtools

