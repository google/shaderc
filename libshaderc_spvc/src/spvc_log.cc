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

namespace {

const char* ToString(LogSeverity severity) {
  switch (severity) {
    case LogSeverity::Debug:
      return "Debug";
    case LogSeverity::Info:
      return "Info";
    case LogSeverity::Warning:
      return "Warning";
    case LogSeverity::Error:
      return "Error";
    default:
      return "";
  }
}

}  // anonymous namespace

LogMessage::LogMessage(shaderc_spvc_context_t context, LogSeverity severity)
    : context_(context), severity_(severity) {}

LogMessage::~LogMessage() {
#if defined(SHADERC_SPVC_DIRECT_LOGGING) || \
    defined(SHADERC_SPVC_CONTEXT_LOGGING)
  std::string message = stream_.str();

  // If this message has been moved, its stream is empty.
  if (message.empty()) {
    return;
  }

  const char* severity_name = ToString(severity_);

  FILE* outputStream = stdout;
  if (severity_ == LogSeverity::Warning || severity_ == LogSeverity::Error) {
    outputStream = stderr;
  }

  stream_.clear();
  stream_ << severity_name << ": " << message.c_str() << std::endl;
  std::string full_message = stream_.str();

#if defined(SHADERC_SPVC_DIRECT_LOGGING)
  // Note: we use fprintf because <iostream> includes static initializers.
  fprintf(outputStream, "%s", full_message.c_str());
  fflush(outputStream);
#endif  // defined(SHADERC_SPVC_DIRECT_LOGGING)

#if defined(SHADERC_SPVC_CONTEXT_LOGGING)
  if (context_) {
    context_->messages.push_back(full_message.c_str());
  }
#endif  // defined(SHADERC_SPVC_CONTEXT_LOGGING)
#endif  // defined(SHADERC_SPVC_DIRECT_LOGGING) ||
        // defined(SHADERC_SPVC_CONTEXT_LOGGING)
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
