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

// Interface inspired by src/common/Log.h from Dawn

// Logging logging is done using the [Debug|Info|Warning|Error]Log() function
// this way:
//
//   InfoLog() << things << that << ostringstream << supports; // No need for a
//   std::endl or "\n"
//
// It creates a LogMessage object that isn't stored anywhere and gets its
// destructor called immediately which outputs the stored ostringstream in the
// right place.

#ifndef LIBSHADERC_SPVC_SRC_SPVC_LOG_H_
#define LIBSHADERC_SPVC_SRC_SPVC_LOG_H_

#include <sstream>

#include "spvc/spvc.h"

namespace shaderc_spvc {

// Log levels mostly used to signal intent where the log message is produced and
// used to route the message to the correct output.
enum class LogSeverity {
  Debug,
  Info,
  Warning,
  Error,
};

// Essentially an ostringstream that will print itself in its destructor.
class LogMessage {
 public:
  LogMessage(shaderc_spvc_context_t context, LogSeverity severity);
  ~LogMessage();

  template <typename T>
  LogMessage& operator<<(T&& value) {
    stream_ << value;
    return *this;
  }

 private:
  // Disallow copy and assign
  LogMessage(const LogMessage& other) = delete;
  LogMessage& operator=(const LogMessage& other) = delete;

#if defined(SHADERC_SPVC_DISABLE_CONTEXT_LOGGING)
  shaderc_spvc_context_t context_ __attribute__((unused));
#else
  shaderc_spvc_context_t context_;
#endif  // defined(SHADERC_SPVC_DISABLE_CONTEXT_LOGGING)
  std::ostringstream stream_;
  LogSeverity severity_;
};

// Short-hands to create a LogMessage with the respective severity.
LogMessage DebugLog(shaderc_spvc_context_t context);
LogMessage InfoLog(shaderc_spvc_context_t context);
LogMessage WarningLog(shaderc_spvc_context_t context);
LogMessage ErrorLog(shaderc_spvc_context_t context);

}  // namespace shaderc_spvc

#endif  // LIBSHADERC_SPVC_SRC_SPVC_LOG_H_
