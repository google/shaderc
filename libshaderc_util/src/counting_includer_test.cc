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

#include "libshaderc_util/counting_includer.h"

#include <gmock/gmock.h>
#include <thread>

namespace {

// A trivial implementation of CountingIncluder's virtual methods, so tests can
// instantiate.
class ConcreteCountingIncluder : public shaderc_util::CountingIncluder {
 public:
  std::pair<std::string, std::string> include_delegate(
      const char* filename) const override {
    return std::make_pair<std::string, std::string>("", "Unexpected #include");
  }
};

TEST(CountingIncluderTest, InitialCount) {
  EXPECT_EQ(0, ConcreteCountingIncluder().num_include_directives());
}

TEST(CountingIncluderTest, OneInclude) {
  ConcreteCountingIncluder includer;
  includer.include("random file name");
  EXPECT_EQ(1, includer.num_include_directives());
}

TEST(CountingIncluderTest, TwoIncludes) {
  ConcreteCountingIncluder includer;
  includer.include("name1");
  includer.include("name2");
  EXPECT_EQ(2, includer.num_include_directives());
}

TEST(CountingIncluderTest, ManyIncludes) {
  ConcreteCountingIncluder includer;
  for (int i = 0; i < 100; ++i) {
    includer.include("filename");
  }
  EXPECT_EQ(100, includer.num_include_directives());
}

TEST(CountingIncluderTest, ThreadedIncludes) {
  ConcreteCountingIncluder includer;
  std::thread t1([&includer]() { includer.include("name1"); });
  std::thread t2([&includer]() { includer.include("name2"); });
  std::thread t3([&includer]() { includer.include("name3"); });
  t1.join();
  t2.join();
  t3.join();
  EXPECT_EQ(3, includer.num_include_directives());
}

}  // anonymous namespace
