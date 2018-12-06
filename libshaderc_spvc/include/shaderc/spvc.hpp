// Copyright 2018 The Shaderc Authors. All rights reserved.
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

#ifndef SHADERC_SPVC_SPVC_HPP_
#define SHADERC_SPVC_SPVC_HPP_

#include <memory>
#include <string>
#include <vector>

#include "spvc.h"

namespace spvc {
// A CompilationResult contains the compiler output, compilation status,
// and messages.
//
// The compiler output is stored as an array of elements and accessed
// via random access iterators provided by cbegin() and cend().  The iterators
// are contiguous in the sense of "Contiguous Iterators: A Refinement of
// Random Access Iterators", Nevin Liber, C++ Library Evolution Working
// Group Working Paper N3884.
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3884.pdf
//
// Methods begin() and end() are also provided to enable range-based for.
// They are synonyms to cbegin() and cend(), respectively.
template <typename OutputElementType>
class CompilationResult {
 public:
  typedef OutputElementType element_type;
  // The type used to describe the begin and end iterators on the
  // compiler output.
  typedef const OutputElementType* const_iterator;

  // Upon creation, the CompilationResult takes ownership of the
  // spvc_compilation_result instance. During destruction of the
  // CompilationResult, the spvc_compilation_result will be released.
  explicit CompilationResult(spvc_compilation_result_t compilation_result)
      : compilation_result_(compilation_result) {}
  CompilationResult() : compilation_result_(nullptr) {}
  ~CompilationResult() { spvc_result_release(compilation_result_); }

  CompilationResult(CompilationResult&& other) : compilation_result_(nullptr) {
    *this = std::move(other);
  }

  CompilationResult& operator=(CompilationResult&& other) {
    if (compilation_result_) {
      spvc_result_release(compilation_result_);
    }
    compilation_result_ = other.compilation_result_;
    other.compilation_result_ = nullptr;
    return *this;
  }

  // Returns any error message found during compilation.
  std::string GetErrorMessage() const {
    if (!compilation_result_) {
      return "";
    }
    return spvc_result_get_error_message(compilation_result_);
  }

  // Returns the compilation status, indicating whether the compilation
  // succeeded, or failed due to some reasons, like invalid shader stage or
  // compilation errors.
  spvc_compilation_status GetCompilationStatus() const {
    if (!compilation_result_) {
      return spvc_compilation_status_null_result_object;
    }
    return spvc_result_get_compilation_status(compilation_result_);
  }

  // Returns a random access (contiguous) iterator pointing to the start
  // of the compilation output.  It is valid for the lifetime of this object.
  // If there is no compilation result, then returns nullptr.
  const_iterator cbegin() const {
    if (!compilation_result_) return nullptr;
    return reinterpret_cast<const_iterator>(
        spvc_result_get_bytes(compilation_result_));
  }

  // Returns a random access (contiguous) iterator pointing to the end of
  // the compilation output.  It is valid for the lifetime of this object.
  // If there is no compilation result, then returns nullptr.
  const_iterator cend() const {
    if (!compilation_result_) return nullptr;
    return cbegin() +
           spvc_result_get_length(compilation_result_) /
               sizeof(OutputElementType);
  }

  // Returns the same iterator as cbegin().
  const_iterator begin() const { return cbegin(); }
  // Returns the same iterator as cend().
  const_iterator end() const { return cend(); }

  // Returns the number of warnings generated during the compilation.
  size_t GetNumWarnings() const {
    if (!compilation_result_) {
      return 0;
    }
    return spvc_result_get_num_warnings(compilation_result_);
  }

  // Returns the number of errors generated during the compilation.
  size_t GetNumErrors() const {
    if (!compilation_result_) {
      return 0;
    }
    return spvc_result_get_num_errors(compilation_result_);
  }

 private:
  CompilationResult(const CompilationResult& other) = delete;
  CompilationResult& operator=(const CompilationResult& other) = delete;

  spvc_compilation_result_t compilation_result_;
};

// A compilation result for a SPIR-V binary module, which is an array
// of uint32_t words.
using SpvCompilationResult = CompilationResult<uint32_t>;
// A compilation result in SPIR-V assembly syntax.
using AssemblyCompilationResult = CompilationResult<char>;
// Preprocessed source text.
using PreprocessedSourceCompilationResult = CompilationResult<char>;

// Contains any options that can have default values for a compilation.
class CompileOptions {
 public:
  CompileOptions() { options_ = spvc_compile_options_initialize(); }
  ~CompileOptions() { spvc_compile_options_release(options_); }
  CompileOptions(const CompileOptions& other) {
    options_ = spvc_compile_options_clone(other.options_);
  }
  CompileOptions(CompileOptions&& other) {
    options_ = other.options_;
    other.options_ = nullptr;
  }

 private:
  CompileOptions& operator=(const CompileOptions& other) = delete;
  spvc_compile_options_t options_;
  //std::unique_ptr<IncluderInterface> includer_;

  friend class Compiler;
};

// The compilation context for compiling source to SPIR-V.
class Compiler {
 public:
  Compiler() : compiler_(spvc_compiler_initialize()) {}
  ~Compiler() { spvc_compiler_release(compiler_); }

  Compiler(Compiler&& other) {
    compiler_ = other.compiler_;
    other.compiler_ = nullptr;
  }

  bool IsValid() const { return compiler_ != nullptr; }

  // Compiles the given source SPIR-V to GLSL.
  SpvCompilationResult CompileSpvToGlsl(const uint32_t* source,
                                        size_t source_len,
                                        const CompileOptions& options) const {
    spvc_compilation_result_t compilation_result = spvc_compile_into_glsl(
        compiler_, source, source_len, options.options_);
    return SpvCompilationResult(compilation_result);
  }

 private:
  Compiler(const Compiler&) = delete;
  Compiler& operator=(const Compiler& other) = delete;

  spvc_compiler_t compiler_;
};
}  // namespace spvc

#endif  // SHADERC_SPVC_SPVC_HPP_
