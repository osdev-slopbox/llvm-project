//===--- ParserTest.cpp - Ada parser unit tests -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/Parser/AdaParser.h"
#include "gtest/gtest.h"

using namespace clang;
using namespace clang::ada;

namespace {

// Helper to parse source and check for errors
std::unique_ptr<LibraryUnit> parseSource(const std::string &Source) {
  AdaParser Parser("test.adb", Source);
  return Parser.ParseCompilationUnit();
}

TEST(ParserTest, SimpleProcedure) {
  auto Decl = parseSource("procedure Hello is begin null; end Hello;");
  EXPECT_NE(Decl, nullptr);
}

TEST(ParserTest, ProcedureWithParameters) {
  auto Decl = parseSource(
      "procedure Add(A, B : Integer) is begin null; end Add;");
  EXPECT_NE(Decl, nullptr);
}

TEST(ParserTest, FunctionWithReturn) {
  auto Decl = parseSource(
      "function Double(X : Integer) return Integer is begin return 0; end Double;");
  EXPECT_NE(Decl, nullptr);
}

TEST(ParserTest, Package) {
  auto Decl = parseSource(
      "package My_Pkg is procedure Foo; end My_Pkg;");
  EXPECT_NE(Decl, nullptr);
}

TEST(ParserTest, ObjectDeclaration) {
  auto Decl = parseSource(
      "procedure P is X : Integer := 42; begin null; end P;");
  EXPECT_NE(Decl, nullptr);
}

TEST(ParserTest, IfStatement) {
  auto Decl = parseSource(
      "procedure P(X : Integer) is begin if X > 0 then null; end if; end P;");
  EXPECT_NE(Decl, nullptr);
}

TEST(ParserTest, WhileLoop) {
  auto Decl = parseSource(
      "procedure P is begin while true loop exit; end loop; end P;");
  EXPECT_NE(Decl, nullptr);
}

TEST(ParserTest, ForLoop) {
  auto Decl = parseSource(
      "procedure P is begin for I in 1 .. 10 loop null; end loop; end P;");
  EXPECT_NE(Decl, nullptr);
}

TEST(ParserTest, CaseStatement) {
  auto Decl = parseSource(
      "procedure P(X : Integer) is begin case X is when 1 => null; when others => null; end case; end P;");
  EXPECT_NE(Decl, nullptr);
}

TEST(ParserTest, ExceptionHandling) {
  auto Decl = parseSource(
      "procedure P is begin raise Constraint_Error; exception when E => null; end P;");
  EXPECT_NE(Decl, nullptr);
}

} // namespace