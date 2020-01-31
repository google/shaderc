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

const char* SeverityName(LogSeverity severity) {
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
  std::string fullMessage = stream_.str();

  // If this message has been moved, its stream is empty.
  if (fullMessage.empty()) {
    return;
  }

  const char* severityName = SeverityName(severity_);

  FILE* outputStream = stdout;
  if (severity_ == LogSeverity::Warning || severity_ == LogSeverity::Error) {
    outputStream = stderr;
  }

  std::unique_ptr<char> message;
  int count =
      snprintf(message.get(), 0, "%s: %s\n", severityName, fullMessage.c_str());
  message.reset(
      static_cast<char*>(malloc((count + 1) * sizeof(char))));  // + 1 for \0
  snprintf(message.get(), count, "%s: %s\n", severityName, fullMessage.c_str());

#if defined(SHADERC_SPVC_DIRECT_LOGGING)
  // Note: we use fprintf because <iostream> includes static initializers.
  fprintf(outputStream, "%s", message.get());
  fflush(outputStream);
#endif  // defined(SHADERC_SPVC_DIRECT_LOGGING)

#if defined(SHADERC_SPVC_CONTEXT_LOGGING)
  if (context_) {
    context_->messages.push_back(message.get());
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

LogMessage DebugLog(shaderc_spvc_context_t context, const char* file,
                    const char* function, int line) {
  return std::move(DebugLog(context) << file << ":" << line << "(" << function << ")");
}

}  // namespace shaderc_spvc
