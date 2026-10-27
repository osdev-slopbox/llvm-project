//===--- Token.h - Ada token representation --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_ADA_COMMON_TOKEN_H
#define CLANG_ADA_COMMON_TOKEN_H

#include <string>
#include <vector>
#include <cstdint>

namespace clang {
namespace ada {

// Ada 2012 token kinds.
// Keywords are case-insensitive (lexer normalizes to lowercase).
// Identifiers are case-sensitive.
enum class TokKind : uint16_t {
  // Literals
  IntegerLiteral = 1,
  RealLiteral,
  CharacterLiteral,
  StringLiteral,
  BooleanLiteral,  // TRUE, FALSE, NULL (case-insensitive)

  // Identifiers
  Identifier,
  Keyword,  // All other keywords stored as Identifier with keyword flag

  // Punctuation
  LParen,        // (
  RParen,        // )
  LBracket,      // [
  RBracket,      // ]
  LAngle,        // <
  RAngle,        // >
  LBrace,        // {
  RBrace,        // }
  Comma,         // ,
  Period,        // .
  Semicolon,     // ;
  Colon,         // :
  Equals,        // =
  Minus,         // -
  Plus,          // +
  Asterisk,      // *
  Slash,         // /
  Ampersand,     // &
  Pipe,          // |
  At,            // @
  Tilde,         // ~
  QuestionMark,  // ?
  Backslash,     // \
  DoubleQuote,   // "
  SingleQuote,   // '
  Backtick,      // `

  // Two-character operators
  LEq,           // <=
  GEq,           // >=
  NEq,           // /=
  Concat,        // & (handled as Ampersand)
  DotDot,        // ..
  Arrow,         // =>

  // Delimiters
  EOF_Tok,
  Error
};

// Token with source location.
struct Token {
  TokKind Kind = TokKind::EOF_Tok;
  std::string Spelling;  // Original source text (case-preserved)
  std::string Text;      // Normalized text (lowercase for keywords)
  unsigned Line = 0;
  unsigned Column = 0;
  bool IsKeyword = false;
};

// Source location for diagnostics.
struct SourceLocation {
  std::string File;
  unsigned Line = 0;
  unsigned Column = 0;
};

} // namespace ada
} // namespace clang

#endif // CLANG_ADA_COMMON_TOKEN_H