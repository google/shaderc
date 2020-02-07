// Copyright 2020 The Shaderc Authors. All rights reserved.
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

#include "spvc_log.h"

#include <cstdio>

#include "spvc_private.h"

namespace shaderc_spvc {

LogMessage::LogMessage(shaderc_spvc_context_t context, LogSeverity severity)
    : context_(context),
      severity_(severity) {
  switch (severity_) {
    case LogSeverity::Debug:
      stream_ << "Debug: ";
      break;
    case LogSeverity::Info:
      stream_ << "Info: ";
      break;
    case LogSeverity::Warning:
      stream_ << "Warning: ";
      break;
    case LogSeverity::Error:
      stream_ << "Error: ";
      break;
  }
}

LogMessage::~LogMessage() {
#if defined(SHADERC_SPVC_ENABLE_DIRECT_LOGGING) || \
    !defined(SHADERC_SPVC_DISABLE_CONTEXT_LOGGING)
  std::string message = stream_.str();

  // If this message has been moved, its stream is empty.
  if (message.empty()) {
    return;
  }

#if defined(SHADERC_SPVC_ENABLE_DIRECT_LOGGING)
  FILE* outputStream = stdout;
  if (severity_ == LogSeverity::Warning || severity_ == LogSeverity::Error) {
    outputStream = stderr;
  }

  // Note: we use fprintf because <iostream> includes static initializers.
  fprintf(outputStream, "%s\n", stream_.str().c_str());
  fflush(outputStream);
#endif  // defined(SHADERC_SPVC_ENABLE_DIRECT_LOGGING)

#if !defined(SHADERC_SPVC_DISABLE_CONTEXT_LOGGING)
  if (context_) {
    context_->messages.push_back(stream_.str());
  }
#endif  // !defined(SHADERC_SPVC_DISABLE_CONTEXT_LOGGING)
#endif  // defined(SHADERC_SPVC_ENABLE_DIRECT_LOGGING) ||
        // !defined(SHADERC_SPVC_DISABLE_CONTEXT_LOGGING)
}

LogMessage DebugLog(shaderc_spvc_context_t context) {
  return {context, LogSeverity::Debug};
}

LogMessage InfoLog(shaderc_spvc_context_t context) {
  return {context, LogSeverity::Info};
}

LogMessage WarningLog(shaderc_spvc_context_t context) {
  return {context, LogSeverity::Warning};
}

LogMessage ErrorLog(shaderc_spvc_context_t context) {
  return {context, LogSeverity::Error};
}

}  // namespace shaderc_spvc
