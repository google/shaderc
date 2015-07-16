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

#include "libshaderc_util/io.h"

#include <gmock/gmock.h>

namespace {

using shaderc_util::ReadFile;
using shaderc_util::WriteFile;

std::string ToString(const std::vector<char>& v) {
  return std::string(v.data(), v.size());
}

class ReadFileTest : public testing::Test {
 protected:
  // A vector to pass to ReadFile.
  std::vector<char> read_data;
};

TEST_F(ReadFileTest, CorrectContent) {
  ASSERT_TRUE(ReadFile("include_file.1", &read_data));
  EXPECT_EQ("The quick brown fox jumps over a lazy dog.\n",
            ToString(read_data));
}

TEST_F(ReadFileTest, EmptyContent) {
  ASSERT_TRUE(ReadFile("dir/subdir/include_file.2", &read_data));
  EXPECT_TRUE(read_data.empty());
}

TEST_F(ReadFileTest, FileNotFound) {
  EXPECT_FALSE(ReadFile("garbage garbage vjoiarhiupo hrfewi", &read_data));
}

TEST_F(ReadFileTest, EmptyFilename) { EXPECT_FALSE(ReadFile("", &read_data)); }

TEST(WriteFileTest, Roundtrip) {
  const std::string content = "random content 12345";
  const std::string filename = "WriteFileTestOutput.tmp";
  ASSERT_TRUE(WriteFile(filename, content));
  std::vector<char> read_data;
  ASSERT_TRUE(ReadFile(filename, &read_data));
  EXPECT_EQ(content, ToString(read_data));
}

}  // anonymous namespace
