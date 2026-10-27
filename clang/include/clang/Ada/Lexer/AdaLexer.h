//===--- AdaLexer.h - Ada 2012 lexer -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Ada 2012 lexer: case-insensitive keywords, adjacency-sensitive operators,
// based/exponent numeric literals, line comments, and the full token set.
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_ADA_LEXER_ADALEXER_H
#define CLANG_ADA_LEXER_ADALEXER_H

#include "../Common/Token.h"
#include <string>

namespace clang {
namespace ada {

class AdaLexer {
public:
  AdaLexer(const std::string &FileName, const std::string &Source);
  ~AdaLexer();

  // Get next token. Returns EOF_Tok at end of input.
  Token NextToken();

  // Peek at next token without consuming.
  Token PeekToken();

  // Reset lexer to beginning of input.
  void Reset();

  // Current position.
  std::string GetFileName() const { return FileName; }
  unsigned GetLine() const { return Line; }
  unsigned GetColumn() const { return Column; }

  // Save/restore lexer position for backtracking (used for generic instantiation detection).
  size_t SavePos() const { return Pos; }
  void RestorePos(size_t SavedPos) { Pos = SavedPos; HaveLookahead = false; }

  // Keyword table.
  static bool IsKeyword(const std::string &Name);
  static TokKind GetKeywordKind(const std::string &Name);

private:
  std::string FileName;
  std::string Source;
  size_t Pos = 0;
  unsigned Line = 1;
  unsigned Column = 1;
  Token Lookahead;
  bool HaveLookahead = false;

  void SkipWhitespaceAndComments();
  Token LexIdentifierOrKeyword();
  Token LexNumber();
  Token LexCharacterLiteral();
  Token LexStringLiteral();
  Token LexOperator();
  Token LexPunctuation();
  Token MakeToken(TokKind Kind, const std::string &Text);
  Token MakeLiteral(TokKind Kind, const std::string &Text);

  char PeekChar() const;
  char Advance();
  void AdvanceTo(size_t NewPos);
};

} // namespace ada
} // namespace clang

#endif // CLANG_ADA_LEXER_ADALEXER_H