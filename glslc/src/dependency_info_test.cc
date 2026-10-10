// Copyright 2026 The Shaderc Authors. All rights reserved.
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

#include "dependency_info.h"

#include <gmock/gmock.h>
#include <sstream>
#include <string>
#include <unordered_set>

namespace {

using glslc::DependencyInfoDumpingHandler;
using glslc::EscapeDepPath;
using glslc::WriteEscapedDepPath;
using testing::Eq;

TEST(DependencyInfoEscapeTest, PlainPathUnchanged) {
  std::ostringstream ss;
  EXPECT_TRUE(WriteEscapedDepPath(ss, "path/to/shader.vert"));
  EXPECT_EQ("path/to/shader.vert", ss.str());
}

TEST(DependencyInfoEscapeTest, SpaceEscaped) {
  std::string escaped;
  EXPECT_TRUE(EscapeDepPath("path with spaces/file name.vert", &escaped));
  EXPECT_EQ("path\\ with\\ spaces/file\\ name.vert", escaped);
}

TEST(DependencyInfoEscapeTest, TabEscapedAsWhitespace) {
  std::string escaped;
  EXPECT_TRUE(EscapeDepPath("path\twith\ttabs.vert", &escaped));
  EXPECT_EQ("path\\ with\\ tabs.vert", escaped);
}

TEST(DependencyInfoEscapeTest, DollarSignDoubled) {
  std::string escaped;
  EXPECT_TRUE(EscapeDepPath("h$(shell touch marker).glsl", &escaped));
  EXPECT_EQ("h$$(shell\\ touch\\ marker).glsl", escaped);
}

TEST(DependencyInfoEscapeTest, HashEscaped) {
  std::string escaped;
  EXPECT_TRUE(EscapeDepPath("path/to/header#tag.glsl", &escaped));
  EXPECT_EQ("path/to/header\\#tag.glsl", escaped);
}

TEST(DependencyInfoEscapeTest, ColonEscaped) {
  std::string escaped;
  EXPECT_TRUE(EscapeDepPath("C:/drive/path.vert", &escaped));
  EXPECT_EQ("C\\:/drive/path.vert", escaped);
}

TEST(DependencyInfoEscapeTest, BracketsEscaped) {
  std::string escaped;
  EXPECT_TRUE(EscapeDepPath("dir/arr[0].glsl", &escaped));
  EXPECT_EQ("dir/arr\\[0\\].glsl", escaped);
}

TEST(DependencyInfoEscapeTest, BackslashEscaped) {
  std::string escaped;
  EXPECT_TRUE(EscapeDepPath("path\\to\\file.vert", &escaped));
  EXPECT_EQ("path\\\\to\\\\file.vert", escaped);
}

TEST(DependencyInfoEscapeTest, CombinedMakeSpecialCharacters) {
  std::string escaped;
  EXPECT_TRUE(EscapeDepPath("C:\\dir name\\$[0]:#test.vert", &escaped));
  EXPECT_EQ("C\\:\\\\dir\\ name\\\\$$\\[0\\]\\:\\#test.vert", escaped);
}

TEST(DependencyInfoEscapeTest, ExactCombinedCharacters) {
  std::string escaped;
  EXPECT_TRUE(EscapeDepPath("$ #:[\\]\t", &escaped));
  EXPECT_EQ("$$\\ \\#\\:\\[\\\\\\]\\ ", escaped);
}

TEST(DependencyInfoEscapeTest, RejectNewline) {
  std::ostringstream ss;
  EXPECT_FALSE(WriteEscapedDepPath(ss, "header\ninjected.glsl"));
  std::string escaped;
  EXPECT_FALSE(EscapeDepPath("header\ninjected.glsl", &escaped));
}

TEST(DependencyInfoEscapeTest, RejectCarriageReturn) {
  std::ostringstream ss;
  EXPECT_FALSE(WriteEscapedDepPath(ss, "header\rinjected.glsl"));
  std::string escaped;
  EXPECT_FALSE(EscapeDepPath("header\rinjected.glsl", &escaped));
}

TEST(DependencyInfoEscapeTest, RejectCRLF) {
  std::ostringstream ss;
  EXPECT_FALSE(WriteEscapedDepPath(ss, "header\r\ninjected.glsl"));
  std::string escaped;
  EXPECT_FALSE(EscapeDepPath("header\r\ninjected.glsl", &escaped));
}

TEST(DependencyInfoDumpTest, CompilationOutputWithEscapedTokens) {
  DependencyInfoDumpingHandler handler;
  handler.SetDumpAsNormalCompilationOutput();
  handler.SetTarget("out put.spv");

  std::string output;
  std::unordered_set<std::string> deps = {"inc$1.h", "inc#2.h"};
  EXPECT_TRUE(handler.DumpDependencyInfo("fallback.spv", "src file.vert",
                                         &output, deps));

  // The output should begin with the escaped target and escaped source.
  EXPECT_THAT(output, testing::StartsWith("out\\ put.spv: src\\ file.vert"));
  // Both escaped dependencies should be present.
  EXPECT_THAT(output, testing::HasSubstr(" inc$$1.h"));
  EXPECT_THAT(output, testing::HasSubstr(" inc\\#2.h"));
  EXPECT_THAT(output, testing::EndsWith("\n"));
}

TEST(DependencyInfoDumpTest, RejectNewlineInTarget) {
  DependencyInfoDumpingHandler handler;
  handler.SetDumpAsNormalCompilationOutput();
  handler.SetTarget("bad\ntarget");

  std::string output = "initial";
  std::unordered_set<std::string> deps;
  EXPECT_FALSE(handler.DumpDependencyInfo("fallback.spv", "src.vert",
                                          &output, deps));
  EXPECT_EQ("initial", output);
}

TEST(DependencyInfoDumpTest, RejectNewlineInSource) {
  DependencyInfoDumpingHandler handler;
  handler.SetDumpAsNormalCompilationOutput();

  std::string output = "initial";
  std::unordered_set<std::string> deps;
  EXPECT_FALSE(handler.DumpDependencyInfo("fallback.spv", "bad\nsource.vert",
                                          &output, deps));
  EXPECT_EQ("initial", output);
}

TEST(DependencyInfoDumpTest, RejectNewlineInDependency) {
  DependencyInfoDumpingHandler handler;
  handler.SetDumpAsNormalCompilationOutput();

  std::string output = "initial";
  std::unordered_set<std::string> deps = {"ok.h", "bad\nevil:\n\ttouch marker\nb:.h"};
  EXPECT_FALSE(handler.DumpDependencyInfo("out.spv", "src.vert",
                                          &output, deps));
  EXPECT_EQ("initial", output);
}

TEST(DependencyInfoDumpTest, RejectCarriageReturnInDependency) {
  DependencyInfoDumpingHandler handler;
  handler.SetDumpAsNormalCompilationOutput();

  std::string output = "initial";
  std::unordered_set<std::string> deps = {"bad\rfile.h"};
  EXPECT_FALSE(handler.DumpDependencyInfo("out.spv", "src.vert",
                                          &output, deps));
  EXPECT_EQ("initial", output);
}

}  // namespace
