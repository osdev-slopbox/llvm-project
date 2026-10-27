//===--- AdaLexer.cpp - Ada 2012 lexer implementation -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/Lexer/AdaLexer.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <unordered_set>

using namespace clang;
using namespace clang::ada;

// Ada 2012 keywords (all lowercase, matched case-insensitively).
static const std::unordered_set<std::string> AdaKeywords = {
    "abs", "abstract", "accept", "access", "aliased", "all", "and", "array",
    "at", "begin", "body", "case", "constant", "declare", "delay", "delta",
    "digits", "do", "else", "elsif", "end", "entry", "exception",
    "exit", "for", "function", "generic", "if", "in", "is", "limited", "loop",
    "mod", "new", "not", "null", "of", "or", "others", "out",
    "package", "pragma", "private", "procedure", "protected",
    "raise", "range", "record", "rem", "renames", "requeue",
    "return", "reverse", "select", "separate", "some", "subtype",
    "synchronized", "tagged", "task", "terminate", "then",
    "type", "try", "until", "use", "when", "while", "with", "xor"
};

static const std::unordered_set<std::string> BooleanKeywords = {
    "true", "false", "null"
};

AdaLexer::AdaLexer(const std::string &FileName, const std::string &Source)
    : FileName(FileName), Source(Source) {}

AdaLexer::~AdaLexer() {}

void AdaLexer::Reset() {
  Pos = 0;
  Line = 1;
  Column = 1;
  HaveLookahead = false;
}

bool AdaLexer::IsKeyword(const std::string &Name) {
  std::string Lower = Name;
  std::transform(Lower.begin(), Lower.end(), Lower.begin(), ::tolower);
  return AdaKeywords.count(Lower) > 0 || BooleanKeywords.count(Lower) > 0;
}

TokKind AdaLexer::GetKeywordKind(const std::string &Name) {
  std::string Lower = Name;
  std::transform(Lower.begin(), Lower.end(), Lower.begin(), ::tolower);
  if (BooleanKeywords.count(Lower) > 0) {
    return TokKind::BooleanLiteral;
  }
  return TokKind::Keyword;
}

Token AdaLexer::MakeToken(TokKind Kind, const std::string &Text) {
  Token T;
  T.Kind = Kind;
  T.Spelling = Text;
  T.Text = Text;
  T.Line = Line;
  T.Column = Column;
  return T;
}

Token AdaLexer::MakeLiteral(TokKind Kind, const std::string &Text) {
  Token T = MakeToken(Kind, Text);
  return T;
}

char AdaLexer::PeekChar() const {
  if (Pos >= Source.size()) {
    return '\0';
  }
  return Source[Pos];
}

char AdaLexer::Advance() {
  if (Pos >= Source.size()) {
    return '\0';
  }
  char C = Source[Pos++];
  if (C == '\n') {
    Line++;
    Column = 1;
  } else {
    Column++;
  }
  return C;
}

void AdaLexer::AdvanceTo(size_t NewPos) {
  while (Pos < NewPos && Pos < Source.size()) {
    char C = Source[Pos++];
    if (C == '\n') {
      Line++;
      Column = 1;
    } else {
      Column++;
    }
  }
}

void AdaLexer::SkipWhitespaceAndComments() {
  while (true) {
    char C = PeekChar();
    char NextC = C;
    // Peek ahead without consuming
    if (C != '\0') {
      // Save position and peek
      size_t SavedPos = Pos;
      unsigned SavedLine = Line;
      unsigned SavedColumn = Column;
      Advance();
      NextC = PeekChar();
      Pos = SavedPos;
      Line = SavedLine;
      Column = SavedColumn;
    }
    if (C == '\0') {
      return;
    }
    if (std::isspace(static_cast<unsigned char>(C))) {
      Advance();
      continue;
    }
    if (C == '-' && NextC == '-') {
      // Line comment
      Advance();
      Advance();
      while (PeekChar() != '\0' && PeekChar() != '\n') {
        Advance();
      }
      continue;
    }
    break;
  }
}

Token AdaLexer::LexIdentifierOrKeyword() {
  size_t StartPos = Pos;
  while (true) {
    char C = PeekChar();
    if (C == '_' || std::isalnum(static_cast<unsigned char>(C))) {
      Advance();
    } else {
      break;
    }
  }
  std::string Spelling = Source.substr(StartPos, Pos - StartPos);
  std::string Lower = Spelling;
  std::transform(Lower.begin(), Lower.end(), Lower.begin(), ::tolower);

  if (IsKeyword(Lower)) {
    Token T = MakeToken(GetKeywordKind(Lower), Spelling);
    T.IsKeyword = true;
    T.Text = Lower;
    return T;
  }
  return MakeToken(TokKind::Identifier, Spelling);
}

Token AdaLexer::LexNumber() {
  size_t StartPos = Pos;

  // Handle based literals: #16#FF#
  if (PeekChar() == '#') {
    Advance();
    while (PeekChar() != '#' && PeekChar() != '\0') {
      Advance();
    }
    if (PeekChar() == '#') {
      Advance();
    }
    std::string Text = Source.substr(StartPos, Pos - StartPos);
    return MakeLiteral(TokKind::IntegerLiteral, Text);
  }

  // Regular numeric literal
  while (true) {
    char C = PeekChar();
    if (std::isdigit(static_cast<unsigned char>(C))) {
      Advance();
    } else if (C == '.') {
      // Possible decimal point - advance and check for digit
      Advance();
      if (!std::isdigit(static_cast<unsigned char>(PeekChar()))) {
        // Not a decimal literal; break - the dot is consumed
        break;
      }
    } else {
      break;
    }
  }

  // Exponent
  if (PeekChar() == 'E' || PeekChar() == 'e') {
    Advance();
    if (PeekChar() == '+' || PeekChar() == '-') {
      Advance();
    }
    while (std::isdigit(static_cast<unsigned char>(PeekChar()))) {
      Advance();
    }
  }

  std::string Text = Source.substr(StartPos, Pos - StartPos);
  if (Text.find('.') != std::string::npos || Text.find('E') != std::string::npos ||
      Text.find('e') != std::string::npos) {
    return MakeLiteral(TokKind::RealLiteral, Text);
  }
  return MakeLiteral(TokKind::IntegerLiteral, Text);
}

Token AdaLexer::LexCharacterLiteral() {
  size_t StartPos = Pos;
  Advance(); // Consume opening '
  if (PeekChar() == '\0') {
    std::string Text = Source.substr(StartPos, 1);
    return MakeLiteral(TokKind::Error, Text);
  }
  Advance(); // Consume character
  if (PeekChar() != '\'') {
    std::string Text = Source.substr(StartPos, Pos - StartPos);
    return MakeLiteral(TokKind::Error, Text);
  }
  Advance(); // Consume closing '
  std::string Text = Source.substr(StartPos, Pos - StartPos);
  return MakeLiteral(TokKind::CharacterLiteral, Text);
}

Token AdaLexer::LexStringLiteral() {
  size_t StartPos = Pos;
  Advance(); // Consume opening "
  while (true) {
    char C = PeekChar();
    if (C == '\0') {
      break;
    }
    if (C == '"') {
      // Check for embedded quote ""
      size_t NextPos = Pos + 1;
      if (NextPos < Source.size() && Source[NextPos] == '"') {
        Advance(); // Consume first "
        Advance(); // Consume second "
        continue;
      }
      // Found closing quote
      break;
    }
    Advance();
  }
  Advance(); // Consume closing "
  // Strip quotes from the string literal
  std::string Text = Source.substr(StartPos, Pos - StartPos);
  Text.erase(0, 1);       // Remove leading "
  Text.erase(Text.size() - 1, 1); // Remove trailing "
  return MakeLiteral(TokKind::StringLiteral, Text);
}

Token AdaLexer::LexOperator() {
  char C = Advance();

  switch (C) {
    case '(': return MakeToken(TokKind::LParen, "(");
    case ')': return MakeToken(TokKind::RParen, ")");
    case '[': return MakeToken(TokKind::LBracket, "[");
    case ']': return MakeToken(TokKind::RBracket, "]");
    case '{': return MakeToken(TokKind::LBrace, "{");
    case '}': return MakeToken(TokKind::RBrace, "}");
    case ',': return MakeToken(TokKind::Comma, ",");
    case ';': return MakeToken(TokKind::Semicolon, ";");
    case ':': return MakeToken(TokKind::Colon, ":");
    case '=':
      if (PeekChar() == '>') {
        Advance();
        return MakeToken(TokKind::Arrow, "=>");
      }
      return MakeToken(TokKind::Equals, "=");
    case '-':
      // Ada has no `->` operator; `=>` is the arrow and is handled above.
      return MakeToken(TokKind::Minus, "-");
    case '+': return MakeToken(TokKind::Plus, "+");
    case '*':
      if (PeekChar() == '*') {
        Advance();
        return MakeToken(TokKind::Concat, "**"); // Power operator
      }
      return MakeToken(TokKind::Asterisk, "*");
    case '/':
      if (PeekChar() == '=') {
        Advance();
        return MakeToken(TokKind::NEq, "/=");
      }
      return MakeToken(TokKind::Slash, "/");
    case '<':
      if (PeekChar() == '=') {
        Advance();
        return MakeToken(TokKind::LEq, "<=");
      }
      return MakeToken(TokKind::LAngle, "<");
    case '>':
      if (PeekChar() == '=') {
        Advance();
        return MakeToken(TokKind::GEq, ">=");
      }
      return MakeToken(TokKind::RAngle, ">");
    case '&': return MakeToken(TokKind::Ampersand, "&");
    case '|': return MakeToken(TokKind::Pipe, "|");
    case '@': return MakeToken(TokKind::At, "@");
    case '~': return MakeToken(TokKind::Tilde, "~");
    case '?': return MakeToken(TokKind::QuestionMark, "?");
    case '\\': return MakeToken(TokKind::Backslash, "\\");
    case '.':
      if (PeekChar() == '.') {
        Advance();
        return MakeToken(TokKind::DotDot, "..");
      }
      return MakeToken(TokKind::Period, ".");
    default: return MakeToken(TokKind::Error, std::string(1, C));
  }
}

Token AdaLexer::LexPunctuation() {
  return LexOperator();
}

Token AdaLexer::NextToken() {
  if (HaveLookahead) {
    Token T = Lookahead;
    HaveLookahead = false;
    return T;
  }

  SkipWhitespaceAndComments();

  if (PeekChar() == '\0') {
    return MakeToken(TokKind::EOF_Tok, "");
  }

  char C = PeekChar();

  if (std::isalpha(static_cast<unsigned char>(C)) || C == '_') {
    return LexIdentifierOrKeyword();
  }

  if (std::isdigit(static_cast<unsigned char>(C)) || C == '#') {
    return LexNumber();
  }

  if (C == '\'') {
    // Check if this is a character literal ('A') or an attribute marker.
    // A character literal is a single quote, a character, and a closing quote.
    // An attribute marker is a single quote followed by an identifier name.
    // We can distinguish these by looking ahead: if the next character is
    // a letter and the one after is not a closing quote, it's an attribute
    // marker. Otherwise, it's a character literal.
    if (Pos + 2 < Source.size()) {
      char Middle = Source[Pos + 1];
      char End = Source[Pos + 2];
      if (std::isalpha(static_cast<unsigned char>(Middle)) && End == '\'') {
        // Character literal: 'A'
        return LexCharacterLiteral();
      }
    }
    // Attribute marker: 'Name
    Advance();
    return MakeToken(TokKind::SingleQuote, "'");
  }

  if (C == '"') {
    return LexStringLiteral();
  }

  return LexOperator();
}

Token AdaLexer::PeekToken() {
  if (HaveLookahead) {
    return Lookahead;
  }
  Lookahead = NextToken();
  HaveLookahead = true;
  return Lookahead;
}