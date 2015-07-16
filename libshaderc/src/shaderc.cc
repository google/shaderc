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

#include "shaderc_private.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <mutex>
#include <vector>

#include "SPIRV/GlslangToSpv.h"
#include "glslang/Include/InfoSink.h"
#include "glslang/Include/ShHandle.h"
#include "glslang/MachineIndependent/localintermediate.h"
#include "glslang/Public/ShaderLang.h"

#include "libshaderc_util/resources.h"

#if (defined(_MSC_VER) && !defined(_CPPUNWIND)) || !defined(__EXCEPTIONS)
#define TRY_IF_EXCEPTIONS_ENABLED
#define CATCH_IF_EXCEPTIONS_ENABLED(X) if (0)
#else
#define TRY_IF_EXCEPTIONS_ENABLED try
#define CATCH_IF_EXCEPTIONS_ENABLED(X) catch (X)
#endif

namespace {

// Returns shader stage (ie: vertex, fragment, etc.) corresponding to kind.
EShLanguage GetStage(shaderc_shader_kind kind) {
  switch (kind) {
    case shaderc_glsl_vertex_shader:
      return EShLangVertex;
    case shaderc_glsl_fragment_shader:
      return EShLangFragment;
  }
  assert(0 && "Unhandled shaderc_shader_kind");
  return EShLangVertex;
}

// Produces SPIR-V binary when used in ShCompile().
class SpvGenerator : public TCompiler {
 public:
  // Captures language and a valid result object that subsequent method calls
  // will write to.
  SpvGenerator(EShLanguage language, shaderc_spv_module* result)
      : TCompiler(language, info_sink_), result_(result) {}

  // Generates SPIR-V for root, adding it to the vector captured by the
  // constructor.  Always returns true.
  bool compile(TIntermNode* root, int version = 0,
               EProfile profile = ENoProfile) override {
    glslang::TIntermediate intermediate(getLanguage(), version, profile);
    intermediate.setTreeRoot(root);
    glslang::GlslangToSpv(intermediate, result_->spirv);
    return true;  // No failure-reporting mechanism above, so report success.
  }

  // Adds messages accumulated during ShCompile() to the captured result.  This
  // can't be done during compile() because some errors will cause early exit
  // without ever invoking compile().
  void CaptureMessages() { result_->messages = info_sink_.info.c_str(); }

 private:
  // Where to store the compilation results.
  shaderc_spv_module* result_;
  // Collects info & debug messages.
  TInfoSink info_sink_;
};

struct {
  // The number of currently initialized compilers.
  int compiler_initialization_count;

  std::mutex mutex;  // Guards creation and deletion of glsl state.
} glsl_state = {
    0,
};

std::mutex compile_mutex;  // Guards shaderc_compile_*.

}  // anonymous namespace

shaderc_compiler_t shaderc_compiler_initialize() {
  std::lock_guard<std::mutex> lock(glsl_state.mutex);
  ++glsl_state.compiler_initialization_count;
  bool success = true;
  if (glsl_state.compiler_initialization_count == 1) {
    TRY_IF_EXCEPTIONS_ENABLED { success = ShInitialize(); }
    CATCH_IF_EXCEPTIONS_ENABLED(...) { success = false; }
  }
  if (!success) {
    glsl_state.compiler_initialization_count -= 1;
    return nullptr;
  }
  shaderc_compiler_t compiler = new (std::nothrow) shaderc_compiler;
  if (!compiler) {
    return nullptr;
  }

  return compiler;
}

void shaderc_compiler_release(shaderc_compiler_t compiler) {
  if (compiler == nullptr) {
    return;
  }

  if (compiler->initialized) {
    compiler->initialized = false;
    delete compiler;
    // Defend against further dereferences through the "compiler" variable.
    compiler = nullptr;
    std::lock_guard<std::mutex> lock(glsl_state.mutex);
    if (glsl_state.compiler_initialization_count) {
      --glsl_state.compiler_initialization_count;
      if (glsl_state.compiler_initialization_count == 0) {
        TRY_IF_EXCEPTIONS_ENABLED { ShFinalize(); }
        CATCH_IF_EXCEPTIONS_ENABLED(...) {}
      }
    }
  }
}

shaderc_spv_module_t shaderc_compile_into_spv(shaderc_compiler_t compiler,
                                              const char* source_text,
                                              int source_text_size,
                                              shaderc_shader_kind shader_kind,
                                              const char* entry_point_name) {
  std::lock_guard<std::mutex> lock(compile_mutex);
  shaderc_spv_module_t result = new (std::nothrow) shaderc_spv_module;
  if (!result) {
    return nullptr;
  }

  result->compilation_succeeded = false;  // In case we exit early.
  if (!compiler->initialized) return result;
  if (source_text_size < 0) return result;
  TRY_IF_EXCEPTIONS_ENABLED {
    SpvGenerator spv_generator(GetStage(shader_kind), result);
    result->compilation_succeeded = ShCompile(
        static_cast<ShHandle>(&spv_generator), &source_text,
        1 /* only one string */, &source_text_size, EShOptNone,
        &shaderc_util::kDefaultTBuiltInResource, 0 /* no debug options */);
    spv_generator.CaptureMessages();
  }
  CATCH_IF_EXCEPTIONS_ENABLED(...) { result->compilation_succeeded = false; }
  return result;
}

bool shaderc_module_get_success(const shaderc_spv_module_t module) {
  return module->compilation_succeeded;
}

size_t shaderc_module_get_length(const shaderc_spv_module_t module) {
  return module->spirv.size() * sizeof(module->spirv.front());
}

const char* shaderc_module_get_bytes(const shaderc_spv_module_t module) {
  return static_cast<const char*>(
      static_cast<const void*>(module->spirv.data()));
}

void shaderc_module_release(shaderc_spv_module_t module) { delete module; }

const char* shaderc_module_get_error_message(
    const shaderc_spv_module_t module) {
  return module->messages.c_str();
}
