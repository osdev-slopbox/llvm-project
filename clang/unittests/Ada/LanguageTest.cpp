//===--- LanguageTest.cpp - Language string mapping tests ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/LangStandard.h"
#include "gtest/gtest.h"

using namespace clang;

namespace {

TEST(LanguageTest, Ada) {
  EXPECT_EQ("Ada", languageToString(Language::Ada));
}

} // namespace
