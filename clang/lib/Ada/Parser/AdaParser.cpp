//===--- AdaParser.cpp - Ada 2012 parser implementation ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/Parser/AdaParser.h"
#include "clang/Ada/Lexer/AdaLexer.h"
#include <algorithm>
#include <cctype>
#include <iostream>

using namespace clang;
using namespace clang::ada;

AdaParser::AdaParser(const std::string &FileName, const std::string &Source)
    : FileName(FileName), Source(Source) {
  Lexer = std::make_unique<AdaLexer>(FileName, Source);
  CurrentToken = Lexer->NextToken();
  PeekToken = Lexer->NextToken();
}

void AdaParser::Advance() {
  CurrentToken = PeekToken;
  PeekToken = Lexer->NextToken();
}

bool AdaParser::Match(TokKind Kind) {
  if (CurrentToken.Kind == Kind) {
    Advance();
    return true;
  }
  return false;
}

bool AdaParser::MatchKeyword(const std::string &Keyword) {
  if (CurrentToken.Kind == TokKind::Keyword && CurrentToken.Text == Keyword) {
    Advance();
    return true;
  }
  return false;
}

// Human-readable spelling for a token kind, used in parser diagnostics.
static const char *TokenKindName(TokKind Kind) {
  switch (Kind) {
  case TokKind::IntegerLiteral:   return "integer literal";
  case TokKind::RealLiteral:      return "real literal";
  case TokKind::CharacterLiteral: return "character literal";
  case TokKind::StringLiteral:    return "string literal";
  case TokKind::BooleanLiteral:   return "boolean literal";
  case TokKind::Identifier:       return "identifier";
  case TokKind::Keyword:          return "keyword";
  case TokKind::LParen:           return "'('";
  case TokKind::RParen:           return "')'";
  case TokKind::LBracket:         return "'['";
  case TokKind::RBracket:         return "']'";
  case TokKind::LAngle:           return "'<'";
  case TokKind::RAngle:           return "'>'";
  case TokKind::LBrace:           return "'{'";
  case TokKind::RBrace:           return "'}'";
  case TokKind::Comma:            return "','";
  case TokKind::Period:           return "'.'";
  case TokKind::Semicolon:        return "';'";
  case TokKind::Colon:            return "':'";
  case TokKind::Equals:           return "'='";
  case TokKind::Minus:            return "'-'";
  case TokKind::Plus:             return "'+'";
  case TokKind::Asterisk:         return "'*'";
  case TokKind::Slash:            return "'/'";
  case TokKind::Ampersand:        return "'&'";
  case TokKind::Pipe:             return "'|'";
  case TokKind::At:               return "'@'";
  case TokKind::Tilde:            return "'~'";
  case TokKind::QuestionMark:     return "'?'";
  case TokKind::Backslash:        return "'\\'";
  case TokKind::Backtick:         return "'`'";
  case TokKind::LEq:              return "'<='";
  case TokKind::GEq:              return "'>='";
  case TokKind::NEq:              return "'/='";
  case TokKind::Concat:           return "'&'";
  case TokKind::DotDot:           return "'..'";
  case TokKind::Arrow:            return "'=>'";
  case TokKind::EOF_Tok:          return "end of file";
  case TokKind::Error:            return "invalid token";
  }
  return "token";
}

Token AdaParser::Expect(TokKind Kind) {
  if (CurrentToken.Kind != Kind) {
    // Contextual message: name the expected token and show what was found.
    std::cerr << "AdaParser: expected " << TokenKindName(Kind);
    if (CurrentToken.Kind == TokKind::EOF_Tok) {
      std::cerr << " but reached end of file";
    } else if (!CurrentToken.Spelling.empty()) {
      std::cerr << " but found '" << CurrentToken.Spelling << "'";
    }
    std::cerr << " at line " << CurrentToken.Line << "\n";
  }
  Advance();
  return CurrentToken;
}

Token AdaParser::ExpectKeyword(const std::string &Keyword) {
  if (CurrentToken.Kind != TokKind::Keyword || CurrentToken.Text != Keyword) {
    std::cerr << "AdaParser: expected keyword '" << Keyword << "'";
    if (CurrentToken.Kind == TokKind::EOF_Tok) {
      std::cerr << " but reached end of file";
    } else if (!CurrentToken.Spelling.empty()) {
      std::cerr << " but found '" << CurrentToken.Spelling << "'";
    }
    std::cerr << " at line " << CurrentToken.Line << "\n";
  }
  Advance();
  return CurrentToken;
}

bool AdaParser::AtEnd() {
  return CurrentToken.Kind == TokKind::EOF_Tok;
}

std::unique_ptr<LibraryUnit> AdaParser::ParseLibraryUnit() {
  if (AtEnd()) {
    return std::make_unique<LibraryUnit>();
  }

  // Parse with clauses
  std::vector<std::string> WithClauses;
  while (MatchKeyword("with")) {
    // Parse comma-separated list of (possibly dotted) package names.
    if (CurrentToken.Kind == TokKind::Identifier) {
      std::string Name = CurrentToken.Spelling;
      Advance();
      while (Match(TokKind::Period) &&
             CurrentToken.Kind == TokKind::Identifier) {
        Name += ".";
        Name += CurrentToken.Spelling;
        Advance();
      }
      WithClauses.push_back(Name);
    }
    while (Match(TokKind::Comma)) {
      if (CurrentToken.Kind == TokKind::Identifier) {
        std::string Name = CurrentToken.Spelling;
        Advance();
        while (Match(TokKind::Period) &&
               CurrentToken.Kind == TokKind::Identifier) {
          Name += ".";
          Name += CurrentToken.Spelling;
          Advance();
        }
        WithClauses.push_back(Name);
      }
    }
    Expect(TokKind::Semicolon);
  }

  // Skip until we find a valid library unit declaration start
  // This handles // RUN: directives and other non-Ada content
  while (!AtEnd()) {
    if (CurrentToken.Kind == TokKind::Keyword &&
        (CurrentToken.Text == "package" || CurrentToken.Text == "procedure" ||
         CurrentToken.Text == "function")) {
      break;
    }
    Advance();
  }

  if (AtEnd()) {
    return std::make_unique<LibraryUnit>();
  }

  // Parse the library unit declaration
  std::unique_ptr<LibraryUnit> Unit = std::make_unique<LibraryUnit>();
  Unit->WithClauses = WithClauses;

  if (MatchKeyword("package")) {
    auto Package = ParsePackageDeclaration();
    if (Package) {
      Unit->Name = Package->Name;
      Unit->Root = std::move(Package);
    }
  } else if (CurrentToken.Kind == TokKind::Keyword &&
             (CurrentToken.Text == "procedure" || CurrentToken.Text == "function")) {
    bool IsFunction = CurrentToken.Text == "function";
    Advance();
    auto Subprogram = ParseSubprogramDeclaration(IsFunction);
    if (Subprogram) {
      Unit->Name = Subprogram->Name;
      Unit->Root = std::move(Subprogram);
    }
  } else if (MatchKeyword("generic")) {
    // Generic subprogram specification
    // Parse the generic formal parameters (declarations in the generic clause)
    // until we reach the actual subprogram declaration.
    std::vector<std::unique_ptr<DeclarationNode>> GenericDecls;
    while (!AtEnd()) {
      // Stop at the actual generic subprogram declaration
      if (CurrentToken.Kind == TokKind::Keyword &&
          (CurrentToken.Text == "procedure" || CurrentToken.Text == "function")) {
        break;
      }
      if (!IsDeclarationStart()) {
        break;
      }
      auto Decl = ParseDeclaration();
      if (Decl) {
        GenericDecls.push_back(std::move(Decl));
      }
    }
    // Now parse the subprogram declaration
    if (CurrentToken.Kind == TokKind::Keyword &&
        (CurrentToken.Text == "procedure" || CurrentToken.Text == "function")) {
      bool IsFunction = CurrentToken.Text == "function";
      Advance();
      auto Subprogram = ParseSubprogramDeclaration(IsFunction);
      if (Subprogram) {
        Subprogram->IsGeneric = true;
        // Store generic declarations in the subprogram's Declarations
        Subprogram->Declarations = std::move(GenericDecls);
        Unit->Name = Subprogram->Name;
        Unit->Root = std::move(Subprogram);
      }
    }
    // Check for generic body
    if (CurrentToken.Kind == TokKind::Keyword &&
        (CurrentToken.Text == "procedure" || CurrentToken.Text == "function")) {
      bool IsFunction = CurrentToken.Text == "function";
      Advance();
      auto Body = ParseSubprogramDeclaration(IsFunction);
      if (Body) {
        Body->IsBody = true;
        // Attach body to the generic declaration
        if (Unit->Root) {
          auto *Decl = static_cast<SubprogramDeclaration *>(Unit->Root.get());
          // Store body in the declaration's BodyStatements
          Decl->BodyStatements = std::move(Body->BodyStatements);
          Decl->Declarations = std::move(Body->Declarations);
          Decl->IsBody = true;
        }
      }
    }
  }

  return Unit;
}

std::unique_ptr<LibraryUnit> AdaParser::ParseCompilationUnit() {
  return ParseLibraryUnit();
}

void AdaParser::DumpAST() {
  // TODO: Implement AST dump
}

std::vector<std::unique_ptr<DeclarationNode>> AdaParser::ParseDeclarations() {
  std::vector<std::unique_ptr<DeclarationNode>> Declarations;

  while (!AtEnd()) {
    if (!IsDeclarationStart()) {
      break;
    }

    auto Decl = ParseDeclaration();
    if (Decl) {
      Declarations.push_back(std::move(Decl));
    }
  }

  return Declarations;
}

std::unique_ptr<DeclarationNode> AdaParser::ParseDeclaration() {
  if (MatchKeyword("package")) {
    return ParsePackageDeclaration();
  } else if (CurrentToken.Kind == TokKind::Keyword &&
             (CurrentToken.Text == "procedure" || CurrentToken.Text == "function")) {
    bool IsFunction = CurrentToken.Text == "function";
    Advance();
    // Check if this is a generic instantiation: name is new
    if (CurrentToken.Kind == TokKind::Identifier) {
      std::string Name = CurrentToken.Spelling;
      Advance();
      if (MatchKeyword("is")) {
        if (MatchKeyword("new")) {
          return ParseGenericInstantiation(IsFunction, Name);
        }
        // Not an instantiation, but we've consumed "name is"
        // Parse as regular subprogram with the name
        return ParseSubprogramDeclaration(IsFunction, Name);
      }
      // Not "is new", parse as regular subprogram with the name
      return ParseSubprogramDeclaration(IsFunction, Name);
    }
    return ParseSubprogramDeclaration(IsFunction);
  } else if (MatchKeyword("type")) {
    return ParseTypeDeclaration();
  } else if (MatchKeyword("constant") || MatchKeyword("variable")) {
    return ParseObjectDeclaration();
  } else if (MatchKeyword("pragma")) {
    return ParsePragma();
  } else if (MatchKeyword("task")) {
    if (MatchKeyword("body")) {
      return ParseTaskBody();
    }
    // We've consumed "task" but not "body"
    // Parse as task declaration
    // Need to back up one token (lexer doesn't support that)
    // Instead, create the declaration here
    auto Task = std::make_unique<TaskDeclaration>();
    if (CurrentToken.Kind == TokKind::Identifier) {
      Task->Name = CurrentToken.Spelling;
      Advance();
    }
    if (MatchKeyword("is")) {
      // Parse entries
      while (!AtEnd()) {
        if (MatchKeyword("entry")) {
          auto Entry = std::make_unique<EntryDeclaration>();
          if (CurrentToken.Kind == TokKind::Identifier) {
            Entry->Name = CurrentToken.Spelling;
            Advance();
          }
          if (Match(TokKind::LParen)) {
            if (!Match(TokKind::RParen)) {
              do {
                auto Params = ParseFormalParameters();
                for (auto &Param : Params) {
                  Entry->FormalParameters.push_back(std::move(Param));
                }
              } while (Match(TokKind::Comma));
              Expect(TokKind::RParen);
            }
          }
          Expect(TokKind::Semicolon);
          Task->Entries.push_back(std::move(Entry));
        } else if (MatchKeyword("begin")) {
          // Task body follows immediately
          // Parse the body and store it
          auto Body = ParseTaskBody();
          Task->Body = std::move(Body);
          break;
        } else if (MatchKeyword("end")) {
          break;
        } else {
          break;
        }
      }
      ExpectKeyword("end");
      if (CurrentToken.Kind == TokKind::Identifier) {
        Advance();
      }
      Expect(TokKind::Semicolon);
    } else {
      Expect(TokKind::Semicolon);
    }
    return Task;
  } else if (CurrentToken.Kind == TokKind::Identifier) {
    // Could be object declaration or something else
    return ParseObjectDeclaration();
  }

  return nullptr;
}

bool AdaParser::IsDeclarationStart() {
  if (AtEnd()) return false;
  if (CurrentToken.Kind == TokKind::Keyword) {
    return CurrentToken.Text == "package" || CurrentToken.Text == "procedure" ||
           CurrentToken.Text == "function" || CurrentToken.Text == "type" ||
           CurrentToken.Text == "constant" || CurrentToken.Text == "variable" ||
           CurrentToken.Text == "pragma";
  }
  if (CurrentToken.Kind == TokKind::Identifier) {
    return true;
  }
  return false;
}

std::unique_ptr<PackageDeclaration> AdaParser::ParsePackageDeclaration() {
  auto Package = std::make_unique<PackageDeclaration>();

  if (CurrentToken.Kind != TokKind::Identifier) {
    std::cerr << "AdaParser: expected package name at line "
              << CurrentToken.Line << "\n";
    return Package;
  }
  Package->Name = CurrentToken.Spelling;
  Advance();

  // Check for body
  if (MatchKeyword("body")) {
    Package->IsBody = true;
  }

  // Check for "is" keyword
  if (MatchKeyword("is")) {
    // Parse declarations
    Package->Declarations = ParseDeclarations();

    // Check for body
    if (MatchKeyword("begin")) {
      Package->BodyStatements = ParseStatements();
    }

    // Expect "end"
    ExpectKeyword("end");

    // Optional package name
    if (CurrentToken.Kind == TokKind::Identifier) {
      Advance();
    }

    // Expect semicolon
    Expect(TokKind::Semicolon);
  } else {
    // Package spec without body - expect semicolon
    Expect(TokKind::Semicolon);
  }

  return Package;
}

std::vector<std::unique_ptr<StatementNode>> AdaParser::ParseStatements() {
  std::vector<std::unique_ptr<StatementNode>> Statements;

  while (!AtEnd()) {
    if (CurrentToken.Kind == TokKind::Keyword && CurrentToken.Text == "end") {
      // Don't consume `end` — leave it for the caller.
      break;
    }
    if (!IsStatementStart()) {
      break;
    }

    auto Stmt = ParseStatement();
    if (Stmt) {
      Statements.push_back(std::move(Stmt));
    }
  }

  return Statements;
}

bool AdaParser::IsStatementStart() {
  if (AtEnd()) return false;
  if (CurrentToken.Kind == TokKind::Keyword) {
    return CurrentToken.Text == "if" || CurrentToken.Text == "loop" ||
           CurrentToken.Text == "for" || CurrentToken.Text == "while" ||
           CurrentToken.Text == "case" || CurrentToken.Text == "block" ||
           CurrentToken.Text == "return" || CurrentToken.Text == "exit" ||
           CurrentToken.Text == "raise" || CurrentToken.Text == "try" ||
           CurrentToken.Text == "pragma" || CurrentToken.Text == "accept" ||
           CurrentToken.Text == "delay" || CurrentToken.Text == "requeue";
  }
  if (CurrentToken.Kind == TokKind::BooleanLiteral &&
      CurrentToken.Text == "null") {
    return true;
  }
  if (CurrentToken.Kind == TokKind::Identifier) {
    return true;
  }
  return false;
}

std::unique_ptr<StatementNode> AdaParser::ParseStatement() {
  if (MatchKeyword("if")) {
    return ParseIfStatement();
  } else if (MatchKeyword("loop")) {
    return ParseLoopStatement();
  } else if (MatchKeyword("for")) {
    return ParseForLoopStatement();
  } else if (MatchKeyword("while")) {
    return ParseWhileLoopStatement();
  } else if (MatchKeyword("case")) {
    return ParseCaseStatement();
  } else if (MatchKeyword("block")) {
    return ParseBlockStatement();
  } else if (MatchKeyword("return")) {
    return ParseReturnStatement();
  } else if (MatchKeyword("exit")) {
    // Exit statement with optional when clause
    auto Stmt = std::make_unique<ExitStatement>();
    if (CurrentToken.Kind == TokKind::Identifier) {
      Stmt->LoopLabel = CurrentToken.Spelling;
      Advance();
    }
    if (MatchKeyword("when")) {
      Stmt->Condition = ParseExpression();
    }
    Expect(TokKind::Semicolon);
    return Stmt;
  } else if (MatchKeyword("raise")) {
    auto Stmt = std::make_unique<RaiseStatement>();
    if (CurrentToken.Kind == TokKind::Identifier) {
      Stmt->Exception = ParseIdentifier();
    }
    Expect(TokKind::Semicolon);
    return Stmt;
  } else if (MatchKeyword("try")) {
    return ParseTryStatement();
  } else if (MatchKeyword("accept")) {
    return ParseAcceptStatement();
  } else if (MatchKeyword("delay")) {
    // Delay statement: delay <expression>;
    auto Stmt = std::make_unique<DelayStatement>();
    Stmt->Duration = ParseExpression();
    Expect(TokKind::Semicolon);
    return Stmt;
  } else if (MatchKeyword("pragma")) {
    // Pragma as a statement (RM 5.1). Parse with the shared pragma parser and
    // wrap the result so semantic analysis can validate the pragma name.
    auto Decl = ParsePragma();
    auto Stmt = std::make_unique<PragmaStatement>();
    Stmt->PragmaName = Decl->PragmaName;
    Stmt->Arguments = std::move(Decl->Arguments);
    return Stmt;
  } else if (MatchKeyword("requeue")) {
    // Requeue statement: requeue <entry_name> [with abort];
    auto Stmt = std::make_unique<RequeueStatement>();
    if (CurrentToken.Kind == TokKind::Identifier) {
      Stmt->EntryName = CurrentToken.Spelling;
      Advance();
    }
    MatchKeyword("with");
    MatchKeyword("abort");
    Expect(TokKind::Semicolon);
    return Stmt;
  } else if (CurrentToken.Kind == TokKind::BooleanLiteral &&
             CurrentToken.Text == "null") {
    Advance();
    auto Stmt = std::make_unique<NullStatement>();
    Expect(TokKind::Semicolon);
    return Stmt;
  } else if (CurrentToken.Kind == TokKind::Identifier) {
    // Ada assignment targets may be dotted names (P.X), so the `:=`
    // separator is not necessarily the token right after the identifier.
    // Parse the name prefix first, then look at what follows to decide
    // between assignment and procedure call.
    auto Prefix = ParseIdentifier();
    // Statement label: `L: statement` (RM 5.1). A label is a Colon that is
    // not the start of the assignment compound delimiter `:=`.
    if (CurrentToken.Kind == TokKind::Colon &&
        PeekToken.Kind != TokKind::Equals) {
      Advance(); // ':'
      return ParseStatement();
    }
    if (CurrentToken.Kind == TokKind::Colon &&
        PeekToken.Kind == TokKind::Equals) {
      auto Assignment = std::make_unique<AssignmentStatement>();
      Assignment->Target = std::move(Prefix);
      Advance(); // ':'
      Advance(); // '='
      Assignment->Value = ParseExpression();
      Expect(TokKind::Semicolon);
      return Assignment;
    }
    return ParseProcedureCallStatement(std::move(Prefix));
  }

  return nullptr;
}

// Subprogram declarations
std::unique_ptr<SubprogramDeclaration> AdaParser::ParseSubprogramDeclaration(
    bool IsFunction, const std::string &Name) {
  auto Subprogram = std::make_unique<SubprogramDeclaration>();

  if (IsFunction) {
    Subprogram->IsFunction = true;
  }

  // Subprogram name (may have been consumed by caller for instantiation detection)
  if (!Name.empty()) {
    Subprogram->Name = Name;
  } else if (CurrentToken.Kind != TokKind::Identifier) {
    std::cerr << "AdaParser: expected subprogram name at line "
              << CurrentToken.Line << " (got " << (int)CurrentToken.Kind << " "
              << CurrentToken.Spelling << ")\n";
    return Subprogram;
  } else {
    Subprogram->Name = CurrentToken.Spelling;
    Advance();
  }

  // Check for generic parameters (simplified)
  if (Match(TokKind::LParen)) {
    if (!Match(TokKind::RParen)) {
      do {
        auto Params = ParseFormalParameters();
        for (auto &Param : Params) {
          Subprogram->FormalParameters.push_back(std::move(Param));
        }
      } while (Match(TokKind::Semicolon));
      Expect(TokKind::RParen);
    }
  }

  // Check for return type (functions only)
  if (Subprogram->IsFunction && MatchKeyword("return")) {
    Subprogram->ReturnType = ParseType();
  }

  // Check for aspect specification
  if (MatchKeyword("with")) {
    Subprogram->Aspects = ParseAspects();
  }

  // Check for "is" keyword (start of body or abstract declaration)
  if (MatchKeyword("is")) {
    // Check for abstract
    if (MatchKeyword("abstract")) {
      Subprogram->IsAbstract = true;
      Expect(TokKind::Semicolon);
      return Subprogram;
    }
    // Otherwise it's a body - the "is" was already consumed
    // Parse declarations in body
    Subprogram->Declarations = ParseDeclarations();

    // Check for begin
    if (MatchKeyword("begin")) {
      Subprogram->BodyStatements = ParseStatements();
    }

    // Subprogram-level exception handlers (RM 8.6(4)):
    //   exception
    //     when Name is <statements>
    //     ...
    // Parsed into Subprogram->ExceptionHandlers; codegen wraps the
    // body in the same longjmp mechanism as try/except.
    if (MatchKeyword("exception")) {
      ParseExceptionHandlers(Subprogram->ExceptionHandlers);
    }

    // Expect end
    ExpectKeyword("end");

    // Optional name
    if (CurrentToken.Kind == TokKind::Identifier) {
      Advance();
    }

    Expect(TokKind::Semicolon);
  } else {
    // No body - declaration only
    Expect(TokKind::Semicolon);
  }

  return Subprogram;
}

// Object declarations
std::unique_ptr<ObjectDeclaration> AdaParser::ParseObjectDeclaration() {
  auto Object = std::make_unique<ObjectDeclaration>();

  bool IsConstant = MatchKeyword("constant");
  bool IsVariable = MatchKeyword("variable");

  // If we didn't match constant or variable, check if we're at an identifier
  // (object declaration without explicit variable/constant)
  if (!IsConstant && !IsVariable && CurrentToken.Kind == TokKind::Identifier) {
    // Could be object declaration without type
    IsVariable = true;
  }

  Object->IsConstant = IsConstant;

  // Object name(s) - could be multiple
  if (CurrentToken.Kind != TokKind::Identifier) {
    std::cerr << "AdaParser: expected object name at line "
              << CurrentToken.Line << "\n";
    return Object;
  }
  Object->Name = CurrentToken.Spelling;
  Object->Names.push_back(Object->Name);
  Advance();

  // Handle multiple declarators (X, Y : Integer)
  while (Match(TokKind::Comma)) {
    if (CurrentToken.Kind != TokKind::Identifier) {
      std::cerr << "AdaParser: expected object name after comma at line "
                << CurrentToken.Line << "\n";
      break;
    }
    Object->Names.push_back(CurrentToken.Spelling);
    Advance();
  }

  // Check for type
  if (Match(TokKind::Colon)) {
    Object->Type = ParseType();

    // Check for alias
    if (MatchKeyword("is") && MatchKeyword("new")) {
      // Allocation
      Object->Type = ParseType();
    }
  }

  // Check for aspect specification
  if (MatchKeyword("with")) {
    Object->Aspects = ParseAspects();
  }

  // Check for initial value (Ada uses := for initialization)
  if (Match(TokKind::Colon) && Match(TokKind::Equals)) {
    Object->Initializer = ParseExpression();
  }

  Expect(TokKind::Semicolon);

  return Object;
}

// Parse the optional tagged/derived head of a type definition
// (RM 3.9(14)): `[abstract] tagged [new parent_type_name] [private]`,
// plus the forms that complete the definition right there:
//   type D is new P;                   (derived, no extension part)
//   type D is new P private;
//   type T is tagged private;          (tagged private)
//   type T is tagged null record;      (root tagged, empty)
// Sets IsTagged/IsAbstract/IsPrivate/Parent on TypeDecl and creates the
// DerivedType for complete derived definitions. Returns true when the
// definition is complete (the caller then only expects the semicolon);
// the `record ... end record` body forms fall through and the caller
// parses them via ParseType.
bool AdaParser::ParseTaggedHead(TypeDeclaration &TypeDecl) {
  // Consume `null record` (RM 3.7(11) record_definition) when present.
  auto ConsumeNullRecord = [&]() -> bool {
    if (CurrentToken.Kind == TokKind::BooleanLiteral &&
        CurrentToken.Text == "null" &&
        PeekToken.Kind == TokKind::Keyword &&
        PeekToken.Text == "record") {
      Advance(); // null
      Advance(); // record
      return true;
    }
    return false;
  };

  // `abstract` may precede either head (RM 3.9).
  if (MatchKeyword("abstract"))
    TypeDecl.IsAbstract = true;

  // Derived type: `new parent_type_name [private] [extension_part]`
  // (RM 3.9(14)) — no `tagged` keyword. A `record` extension part is
  // only legal for tagged parents, so mark the type tagged here;
  // semantics re-checks the parent.
  if (MatchKeyword("new")) {
    TypeDecl.IsTagged = true;
    if (CurrentToken.Kind == TokKind::Identifier) {
      TypeDecl.Parent = CurrentToken.Spelling;
      Advance();
    }
    // Extension part: `record` follows; the caller parses the body via
    // ParseType and rewraps it as DerivedType::Extensions.
    if (CurrentToken.Kind == TokKind::Keyword &&
        CurrentToken.Text == "record")
      return false;
    // `private` may follow the parent type name (RM 3.9.3).
    if (MatchKeyword("private"))
      TypeDecl.IsPrivate = true;
    // The definition is complete: `type D is new P;` (a `null record`
    // after the parent is also accepted, leniently).
    bool NullRecord = ConsumeNullRecord();
    if (CurrentToken.Kind == TokKind::Semicolon || NullRecord) {
      if (!TypeDecl.Parent.empty() && !TypeDecl.Type) {
        auto Dt = std::make_unique<DerivedType>();
        Dt->ParentType = TypeDecl.Parent;
        TypeDecl.Type = std::move(Dt);
      }
      return true;
    }
    return false;
  }

  bool Tagged = MatchKeyword("tagged");
  if (!Tagged)
    return false;
  TypeDecl.IsTagged = true;
  if (MatchKeyword("abstract"))
    TypeDecl.IsAbstract = true;

  // Root tagged type: `tagged private;` or `tagged null record` complete
  // the definition; `tagged record ... end record` falls through to
  // ParseType.
  if (MatchKeyword("private")) {
    TypeDecl.IsPrivate = true;
    return CurrentToken.Kind == TokKind::Semicolon;
  }
  return ConsumeNullRecord();
}

// Type declarations
std::unique_ptr<TypeDeclaration> AdaParser::ParseTypeDeclaration() {
  auto TypeDecl = std::make_unique<TypeDeclaration>();

  // Type name
  if (CurrentToken.Kind != TokKind::Identifier) {
    std::cerr << "AdaParser: expected type name at line "
              << CurrentToken.Line << "\n";
    return TypeDecl;
  }
  TypeDecl->Name = CurrentToken.Spelling;
  Advance();

  // Check for discriminants
  if (Match(TokKind::LParen)) {
    // Type with discriminants
    auto DiscType = std::make_unique<DiscriminatedType>();
    if (!Match(TokKind::RParen)) {
      do {
        auto Discriminants = ParseFormalParameters();
        for (auto &Disc : Discriminants) {
          DiscType->Discriminants.push_back(std::move(Disc));
        }
      } while (Match(TokKind::Comma));
      Expect(TokKind::RParen);
    }

    if (MatchKeyword("is")) {
      if (MatchKeyword("private")) {
        TypeDecl->IsPrivate = true;
        Expect(TokKind::Semicolon);
        return TypeDecl;
      }
      // Parse record part
      DiscType->Fields = ParseDeclarations();
    }

    TypeDecl->Type = std::move(DiscType);
  } else {
    if (MatchKeyword("is")) {
      if (MatchKeyword("private")) {
        TypeDecl->IsPrivate = true;
        Expect(TokKind::Semicolon);
        return TypeDecl;
      }
      // Tagged head (RM 3.9(14)); for derived types the helper completes
      // the definition on `null record`.
      bool DefinitionComplete = ParseTaggedHead(*TypeDecl);
      if (!DefinitionComplete) {
        // Parse type definition
        TypeDecl->Type = ParseType();
        // Derivation with a record body: the record is the extension
        // part, so attach it to the parent.
        if (TypeDecl->IsTagged && !TypeDecl->Parent.empty() &&
            TypeDecl->Type &&
            TypeDecl->Type->getKind() == NodeKind::RecordType) {
          auto *Rec = static_cast<RecordType *>(TypeDecl->Type.get());
          auto Dt = std::make_unique<DerivedType>();
          Dt->ParentType = TypeDecl->Parent;
          for (auto &F : Rec->Fields)
            Dt->Extensions.push_back(std::move(F));
          TypeDecl->Type = std::move(Dt);
        }
      }
    }
  }

  // Check for aspect specification
  if (MatchKeyword("with")) {
    TypeDecl->Aspects = ParseAspects();
  }

  Expect(TokKind::Semicolon);

  return TypeDecl;
}

// Formal parameters
std::vector<std::unique_ptr<FormalParameter>> AdaParser::ParseFormalParameters() {
  std::vector<std::unique_ptr<FormalParameter>> Params;

  // Parse first parameter name
  if (CurrentToken.Kind != TokKind::Identifier) {
    return Params;
  }

  // Collect all parameter names with the same type
  std::vector<std::string> Names;
  Names.push_back(CurrentToken.Spelling);
  Advance();
  while (Match(TokKind::Comma)) {
    if (CurrentToken.Kind != TokKind::Identifier) {
      break;
    }
    Names.push_back(CurrentToken.Spelling);
    Advance();
  }

  // Check for in/out/in out mode
  bool IsInout = false;
  if (MatchKeyword("in")) {
    if (MatchKeyword("out")) {
      IsInout = true;
    }
  } else if (MatchKeyword("out")) {
    // Out mode
  }

  // Parameter type
  std::unique_ptr<TypeNode> Type;
  std::unique_ptr<ExpressionNode> Default;
  if (Match(TokKind::Colon)) {
    Type = ParseType();
    // Check for default value
    if (Match(TokKind::Colon)) {
      Default = ParseExpression();
    }
  }

  // Create a parameter for each name
  for (const auto &Name : Names) {
    auto Param = std::make_unique<FormalParameter>();
    Param->Name = Name;
    Param->IsInout = IsInout;
    Param->Type = std::move(Type);
    Param->Default = std::move(Default);
    Params.push_back(std::move(Param));
  }

  return Params;
}

// Type parsing
std::unique_ptr<TypeNode> AdaParser::ParseType() {
  if (MatchKeyword("access")) {
    auto AccType = std::make_unique<AccessType>();
    AccType->TargetType = ParseType();
    return std::move(AccType);
  }

  if (MatchKeyword("array")) {
    auto ArrType = std::make_unique<ArrayType>();
    // Simplified array type parsing
    if (Match(TokKind::LParen)) {
      // Index type
      if (MatchKeyword("all")) {
        Expect(TokKind::RParen);
      } else {
        // Parse index range
        Expect(TokKind::RParen);
      }
    }
    ExpectKeyword("of");
    ArrType->ElementType = ParseType();
    return std::move(ArrType);
  }

  if (MatchKeyword("record")) {
    auto RecType = std::make_unique<RecordType>();
    // Parse record components until `end`.
    while (true) {
      if (AtEnd() || MatchKeyword("end")) {
        break;
      }
      if (!IsDeclarationStart()) {
        break;
      }
      auto FieldDecl = ParseObjectDeclaration();
      if (FieldDecl) {
        RecType->Fields.push_back(std::move(FieldDecl));
      }
    }
    if (MatchKeyword("record")) {
      // Explicit `end record`
    }
    // The caller consumes the trailing semicolon.
    return std::move(RecType);
  }

  if (MatchKeyword("string")) {
    return std::make_unique<StringType>();
  }

  // Scalar type or named type
  if (IsTypeStart()) {
    return ParseScalarType();
  }

  // Named type
  if (CurrentToken.Kind == TokKind::Identifier) {
    std::string TypeName = CurrentToken.Spelling;
    Advance();

    // Named type
    auto NamedType = std::make_unique<DerivedType>();
    NamedType->ParentType = TypeName;
    return std::move(NamedType);
  }

  // Check for range
  if (Match(TokKind::DotDot)) {
    // Range constraint
  }

  // No type found
  return nullptr;
}

std::unique_ptr<ScalarType> AdaParser::ParseScalarType() {
  auto Scalar = std::make_unique<ScalarType>();

  if (MatchKeyword("integer")) {
    Scalar->ScalarKind = ScalarType::Integer;
    Scalar->Name = "integer";
  } else if (MatchKeyword("natural")) {
    Scalar->ScalarKind = ScalarType::Integer;
    Scalar->Name = "natural";
  } else if (MatchKeyword("positive")) {
    Scalar->ScalarKind = ScalarType::Integer;
    Scalar->Name = "positive";
  } else if (MatchKeyword("float")) {
    Scalar->ScalarKind = ScalarType::Float;
    Scalar->Name = "float";
  } else if (MatchKeyword("short_float")) {
    Scalar->ScalarKind = ScalarType::Float;
    Scalar->Name = "short_float";
  } else if (MatchKeyword("long_float")) {
    Scalar->ScalarKind = ScalarType::Float;
    Scalar->Name = "long_float";
  } else if (MatchKeyword("character")) {
    Scalar->ScalarKind = ScalarType::Character;
    Scalar->Name = "character";
  } else if (MatchKeyword("wide_character")) {
    Scalar->ScalarKind = ScalarType::Character;
    Scalar->Name = "wide_character";
  } else if (MatchKeyword("wide_wide_character")) {
    Scalar->ScalarKind = ScalarType::Character;
    Scalar->Name = "wide_wide_character";
  } else if (MatchKeyword("boolean")) {
    Scalar->ScalarKind = ScalarType::Boolean;
    Scalar->Name = "boolean";
  } else if (CurrentToken.Kind == TokKind::Identifier) {
    // Check for predefined type names
    std::string TypeName = CurrentToken.Spelling;
    Advance();

    if (TypeName == "Integer" || TypeName == "Natural" || TypeName == "Positive") {
      Scalar->ScalarKind = ScalarType::Integer;
      Scalar->Name = TypeName;
    } else if (TypeName == "Real" || TypeName == "Float" || TypeName == "Long_Float") {
      Scalar->ScalarKind = ScalarType::Float;
      Scalar->Name = TypeName;
    } else if (TypeName == "Boolean") {
      Scalar->ScalarKind = ScalarType::Boolean;
      Scalar->Name = TypeName;
    } else if (TypeName == "Character") {
      Scalar->ScalarKind = ScalarType::Character;
      Scalar->Name = TypeName;
    } else {
      // Named type
      Scalar->Name = TypeName;
      // Default ScalarKind is Integer
    }
  }

  // Check for range
  if (Match(TokKind::DotDot)) {
    // Range constraint
  }

  return Scalar;
}

std::unique_ptr<TypeNode> AdaParser::ParseCompositeType() {
  if (MatchKeyword("array")) {
    auto ArrType = std::make_unique<ArrayType>();
    return std::move(ArrType);
  }

  if (MatchKeyword("record")) {
    auto RecType = std::make_unique<RecordType>();
    return std::move(RecType);
  }

  return nullptr;
}

// Expression parsing
std::unique_ptr<ExpressionNode> AdaParser::ParseExpression() {
  auto Expr = ParseRelationalExpression();

  // And/Or/Xor/Nand/Nor
  while (true) {
    BinaryExpression::BinaryOp Op = BinaryExpression::And; // default
    if (MatchKeyword("and")) {
      Op = BinaryExpression::And;
    } else if (MatchKeyword("or")) {
      Op = BinaryExpression::Or;
    } else if (MatchKeyword("xor")) {
      Op = BinaryExpression::Xor;
    } else if (MatchKeyword("nand")) {
      Op = BinaryExpression::Nand;
    } else if (MatchKeyword("nor")) {
      Op = BinaryExpression::Nor;
    } else {
      break;
    }
    auto Right = ParseRelationalExpression();
    auto Binary = std::make_unique<BinaryExpression>();
    Binary->Left = std::move(Expr);
    Binary->Right = std::move(Right);
    Binary->Op = Op;
    Expr = std::move(Binary);
  }

  return Expr;
}

// RM 4.1(51-52): relational expressions bind tighter than the boolean
// operators, so each operand of and/or/xor/... is itself a relational
// expression (e.g. `A >= 0 and B >= 0`).
std::unique_ptr<ExpressionNode> AdaParser::ParseRelationalExpression() {
  auto Expr = ParseSimpleExpression();

  // Comparisons
  while (true) {
    BinaryExpression::BinaryOp Op = BinaryExpression::Add; // default
    if (Match(TokKind::LEq)) {
      Op = BinaryExpression::LessEqual;
    } else if (Match(TokKind::GEq)) {
      Op = BinaryExpression::GreaterEqual;
    } else if (Match(TokKind::NEq)) {
      Op = BinaryExpression::NotEqual;
    } else if (Match(TokKind::Equals)) {
      Op = BinaryExpression::Equal;
    } else if (Match(TokKind::LAngle)) {
      Op = BinaryExpression::LessThan;
    } else if (Match(TokKind::RAngle)) {
      Op = BinaryExpression::GreaterThan;
    } else {
      break;
    }
    auto Right = ParseSimpleExpression();
    auto Binary = std::make_unique<BinaryExpression>();
    Binary->Left = std::move(Expr);
    Binary->Right = std::move(Right);
    Binary->Op = Op;
    Expr = std::move(Binary);
  }

  // In
  if (MatchKeyword("in")) {
    auto Right = ParseSimpleExpression();
    auto Binary = std::make_unique<BinaryExpression>();
    Binary->Left = std::move(Expr);
    Binary->Right = std::move(Right);
    Binary->Op = BinaryExpression::In;
    Expr = std::move(Binary);
  }

  return Expr;
}

std::unique_ptr<ExpressionNode> AdaParser::ParseSimpleExpression() {
  auto Expr = ParseTerm();

  // Check for sign
  bool IsMinus = Match(TokKind::Minus);
  if (IsMinus || Match(TokKind::Plus)) {
    auto Right = ParseTerm();
    auto Binary = std::make_unique<BinaryExpression>();
    Binary->Left = std::move(Expr);
    Binary->Right = std::move(Right);
    if (IsMinus) {
      Binary->Op = BinaryExpression::Subtract;
    } else {
      Binary->Op = BinaryExpression::Add;
    }
    Expr = std::move(Binary);
  }

  // Concatenation
  while (Match(TokKind::Ampersand)) {
    auto Right = ParseTerm();
    auto Binary = std::make_unique<BinaryExpression>();
    Binary->Left = std::move(Expr);
    Binary->Right = std::move(Right);
    Binary->Op = BinaryExpression::Concat;
    Expr = std::move(Binary);
  }

  return Expr;
}

std::unique_ptr<ExpressionNode> AdaParser::ParseTerm() {
  auto Expr = ParseFactor();

  while (true) {
    BinaryExpression::BinaryOp Op = BinaryExpression::Multiply; // default
    if (Match(TokKind::Asterisk)) {
      Op = BinaryExpression::Multiply;
    } else if (Match(TokKind::Slash)) {
      Op = BinaryExpression::Divide;
    } else if (MatchKeyword("mod")) {
      Op = BinaryExpression::Mod;
    } else if (MatchKeyword("rem")) {
      Op = BinaryExpression::Rem;
    } else {
      break;
    }
    auto Right = ParseFactor();
    auto Binary = std::make_unique<BinaryExpression>();
    Binary->Left = std::move(Expr);
    Binary->Right = std::move(Right);
    Binary->Op = Op;
    Expr = std::move(Binary);
  }

  return Expr;
}

std::unique_ptr<ExpressionNode> AdaParser::ParseFactor() {
  // Check for sign
  if (Match(TokKind::Minus)) {
    auto Factor = ParseFactor();
    auto Unary = std::make_unique<UnaryExpression>();
    Unary->Op = UnaryExpression::Negate;
    Unary->Operand = std::move(Factor);
    return Unary;
  }

  if (Match(TokKind::Plus)) {
    auto Factor = ParseFactor();
    auto Unary = std::make_unique<UnaryExpression>();
    Unary->Op = UnaryExpression::Positive;
    Unary->Operand = std::move(Factor);
    return Unary;
  }

  // Check for not
  if (MatchKeyword("not")) {
    auto Factor = ParseFactor();
    auto Unary = std::make_unique<UnaryExpression>();
    Unary->Op = UnaryExpression::Not;
    Unary->Operand = std::move(Factor);
    return Unary;
  }

  // Parse primary
  auto Expr = ParsePrimary();

  // Check for power (** is tokenized as Concat in Ada)
  if (Match(TokKind::Concat)) {
    auto Right = ParseFactor();
    auto Binary = std::make_unique<BinaryExpression>();
    Binary->Left = std::move(Expr);
    Binary->Right = std::move(Right);
    Binary->Op = BinaryExpression::Power;
    Expr = std::move(Binary);
  }

  return Expr;
}

std::unique_ptr<ExpressionNode> AdaParser::ParsePrimary() {
  // Literals (but not null, which is parsed separately)
  if (CurrentToken.Kind == TokKind::IntegerLiteral ||
      CurrentToken.Kind == TokKind::RealLiteral ||
      CurrentToken.Kind == TokKind::CharacterLiteral ||
      CurrentToken.Kind == TokKind::StringLiteral) {
    return ParseLiteral();
  }

  // Boolean literals (true, false) but not null
  if (CurrentToken.Kind == TokKind::BooleanLiteral &&
      CurrentToken.Text != "null") {
    return ParseLiteral();
  }

  // 'Result attribute (no prefix) - RM 5.5.1(16). In Post aspects,
  // 'Result refers to the function result.
  if (CurrentToken.Kind == TokKind::SingleQuote) {
    Advance();
    if (CurrentToken.Kind == TokKind::Identifier &&
        CurrentToken.Spelling == "Result") {
      auto Attr = std::make_unique<AttributeReference>();
      Attr->AttributeName = "Result";
      Advance();
      return Attr;
    }
    // Not 'Result; fall through to other parsing
  }

  // Parenthesized expression or aggregate
  if (CurrentToken.Kind == TokKind::LParen) {
    // Look ahead: is this a named aggregate (x => ...) or parenthesized expr?
    // We save position and try to parse as aggregate.
    int SavedPos = Lexer->SavePos();
    Advance();  // consume '('
    bool IsAggregate = false;
    if (CurrentToken.Kind == TokKind::Identifier &&
        PeekToken.Kind == TokKind::Arrow) {
      IsAggregate = true;
    }
    if (IsAggregate) {
      // Continue parsing the aggregate
      auto Aggregate = ParseNamedAggregate();
      return Aggregate;
    } else {
      // Not an aggregate; restore and parse as parenthesized expression
      Lexer->RestorePos(SavedPos);
      Match(TokKind::LParen);
      auto Expr = ParseExpression();
      Expect(TokKind::RParen);
      return Expr;
    }
  }

  // Identifier
  if (CurrentToken.Kind == TokKind::Identifier) {
    return ParseIdentifier();
  }

  // Function call
  if (CurrentToken.Kind == TokKind::Identifier &&
      PeekToken.Kind == TokKind::LParen) {
    return ParseIdentifier();
  }

  // Null
  if (MatchKeyword("null")) {
    return std::make_unique<NullExpression>();
  }

  // Allocation expression (new)
  if (MatchKeyword("new")) {
    auto Alloc = std::make_unique<AllocationExpression>();
    Alloc->Type = ParseType();
    // Check for initializer: new T(value) or new T'(aggregate)
    if (CurrentToken.Kind == TokKind::LParen) {
      // Parse the parenthesized expression as the initializer
      Advance();  // consume '('
      Alloc->Initializer = ParseExpression();
      Expect(TokKind::RParen);
    }
    return Alloc;
  }

  // Case expression
  if (MatchKeyword("case")) {
    return ParseCaseExpression();
  }

  // Conditional expression
  if (MatchKeyword("if")) {
    return ParseConditionalExpression();
  }

  return nullptr;
}

std::unique_ptr<ExpressionNode> AdaParser::ParseNamedAggregate() {
  // We've already consumed the '(' and confirmed it's an aggregate.
  // Parse named components: name => expression
  auto Aggregate = std::make_unique<AggregateExpression>();
  Aggregate->AggregateKind = AggregateExpression::Named;

  while (CurrentToken.Kind == TokKind::Identifier) {
    Aggregate->Names.push_back(CurrentToken.Spelling);
    Advance();
    Expect(TokKind::Arrow);
    auto ComponentValue = ParseExpression();
    Aggregate->Elements.push_back(std::move(ComponentValue));
    if (!Match(TokKind::Comma)) {
      break;
    }
  }

  Expect(TokKind::RParen);
  return Aggregate;
}

std::unique_ptr<ExpressionNode> AdaParser::ParseLiteral() {
  auto Literal = std::make_unique<LiteralExpression>();

  if (CurrentToken.Kind == TokKind::IntegerLiteral) {
    Literal->LiteralKind = LiteralExpression::Integer;
    Literal->Value = CurrentToken.Spelling;
    Literal->IntegerValue = std::stoll(CurrentToken.Spelling);
  } else if (CurrentToken.Kind == TokKind::RealLiteral) {
    Literal->LiteralKind = LiteralExpression::Float;
    Literal->Value = CurrentToken.Spelling;
    Literal->FloatValue = std::stod(CurrentToken.Spelling);
  } else if (CurrentToken.Kind == TokKind::CharacterLiteral) {
    Literal->LiteralKind = LiteralExpression::Character;
    Literal->Value = CurrentToken.Spelling;
  } else if (CurrentToken.Kind == TokKind::StringLiteral) {
    Literal->LiteralKind = LiteralExpression::String;
    Literal->Value = CurrentToken.Spelling;
  } else if (CurrentToken.Kind == TokKind::BooleanLiteral) {
    Literal->LiteralKind = LiteralExpression::Boolean;
    Literal->Value = CurrentToken.Spelling;
    if (CurrentToken.Spelling == "TRUE" || CurrentToken.Spelling == "True" ||
        CurrentToken.Spelling == "true") {
      Literal->BooleanValue = true;
    }
  }

  Advance();
  return Literal;
}

  std::unique_ptr<ExpressionNode> AdaParser::ParseIdentifier() {
  auto Identifier = std::make_unique<IdentifierExpression>();
  Identifier->Name = CurrentToken.Spelling;
  Advance();

  // Selected names: package-qualified references such as
  // Ada.Calendar.Clock or Ada.Strings.Fixed.Length. The codegen and runtime
  // are keyed on bare subprogram names, so resolve to the final component.
  {
    std::string Lower = Identifier->Name;
    for (auto &C : Lower)
      C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
    if ((Lower == "ada" || Lower == "interfaces" || Lower == "system" ||
         Lower == "gnat") &&
        CurrentToken.Kind == TokKind::Period) {
      while (CurrentToken.Kind == TokKind::Period) {
        Advance();
        if (CurrentToken.Kind != TokKind::Identifier)
          break;
        Identifier->Name = CurrentToken.Spelling;
        Advance();
      }
    }
  }

  // Check for attribute reference: X'Name (e.g. Result'Image, Add'Result)
  if (CurrentToken.Kind == TokKind::SingleQuote &&
      PeekToken.Kind == TokKind::Identifier) {
    Advance();
    auto Attr = std::make_unique<AttributeReference>();
    Attr->Prefix = std::move(Identifier);
    Attr->AttributeName = CurrentToken.Spelling;
    Advance();
    return Attr;
  }

  // Handle 'Result as a special attribute with no prefix (RM 5.5.1(16)).
  // In Ada, 'Result refers to the function result and can appear without
  // a prefix in Post aspects.
  if (CurrentToken.Kind == TokKind::SingleQuote) {
    Advance();
    if (CurrentToken.Kind == TokKind::Identifier &&
        CurrentToken.Spelling == "Result") {
      auto Attr = std::make_unique<AttributeReference>();
      Attr->AttributeName = "Result";
      Advance();
      return Attr;
    }
  }

  // Check for record field access
  if (Match(TokKind::Period)) {
    if (CurrentToken.Kind == TokKind::Identifier) {
      auto FieldName = CurrentToken.Spelling;
      Advance();
      auto Access = std::make_unique<RecordFieldAccess>();
      Access->Record = std::move(Identifier);
      Access->FieldName = FieldName;
      return Access;
    }
  }

  // Check for function call or type conversion
  if (Match(TokKind::LParen)) {
    std::vector<std::unique_ptr<ExpressionNode>> Args;
    std::vector<std::string> ArgNames;

    if (!Match(TokKind::RParen)) {
      do {
        std::string ArgName;
        // Named association: Name => Expression (RM 6.4(8)).
        if (CurrentToken.Kind == TokKind::Identifier &&
            PeekToken.Kind == TokKind::Arrow) {
          ArgName = CurrentToken.Spelling;
          Advance(); // association name
          Advance(); // '=>'
        }
        auto Arg = ParseExpression();
        if (Arg) {
          Args.push_back(std::move(Arg));
          ArgNames.push_back(ArgName);
        }
      } while (Match(TokKind::Comma));
      Expect(TokKind::RParen);
    }

    // Type conversion: Name(Expression) with a single positional argument and
    // a predefined scalar type name.
    std::string TypeName = Identifier->Name;
    bool IsScalarName =
        TypeName == "Integer" || TypeName == "Natural" ||
        TypeName == "Positive" || TypeName == "Float" ||
        TypeName == "Real" || TypeName == "Long_Float" ||
        TypeName == "Boolean" || TypeName == "Character";
    if (IsScalarName && Args.size() == 1 && ArgNames[0].empty()) {
      auto Conversion = std::make_unique<TypeConversionExpression>();
      if (TypeName == "Integer" || TypeName == "Natural" ||
          TypeName == "Positive") {
        auto IntType = std::make_unique<ScalarType>();
        IntType->ScalarKind = ScalarType::Integer;
        IntType->Name = TypeName;
        Conversion->TargetType = std::move(IntType);
      } else if (TypeName == "Float" || TypeName == "Real" ||
                 TypeName == "Long_Float") {
        auto RealType = std::make_unique<ScalarType>();
        RealType->ScalarKind = ScalarType::Float;
        RealType->Name = TypeName;
        Conversion->TargetType = std::move(RealType);
      } else if (TypeName == "Boolean") {
        auto BoolType = std::make_unique<ScalarType>();
        BoolType->ScalarKind = ScalarType::Boolean;
        BoolType->Name = TypeName;
        Conversion->TargetType = std::move(BoolType);
      } else {
        auto CharType = std::make_unique<ScalarType>();
        CharType->ScalarKind = ScalarType::Character;
        CharType->Name = TypeName;
        Conversion->TargetType = std::move(CharType);
      }
      Conversion->Operand = std::move(Args[0]);
      return Conversion;
    }

    auto Call = std::make_unique<CallExpression>();
    Call->Function = std::move(Identifier);
    Call->Arguments = std::move(Args);
    Call->NamedArguments = std::move(ArgNames);
    return Call;
  }

  return Identifier;
}

// Case expression
std::unique_ptr<ExpressionNode> AdaParser::ParseCaseExpression() {
  auto CaseExpr = std::make_unique<CaseExpression>();
  CaseExpr->Discriminant = ParseExpression();
  ExpectKeyword("of");

  // Parse alternatives
  while (!AtEnd() && !MatchKeyword("end")) {
    // Parse choices
    while (true) {
      auto Choice = ParseExpression();
      if (Choice) {
        CaseExpr->Choices.push_back(std::move(Choice));
      }
      if (!Match(TokKind::Comma)) {
        break;
      }
    }

    ExpectKeyword("=>");
    auto Result = ParseExpression();
    if (Result) {
      CaseExpr->Results.push_back(std::move(Result));
    }
    Expect(TokKind::Semicolon);
  }

  ExpectKeyword("end");
  if (MatchKeyword("case")) {
    // Explicit end case
  }
  Expect(TokKind::Semicolon);

  return CaseExpr;
}

// Conditional expression
std::unique_ptr<ExpressionNode> AdaParser::ParseConditionalExpression() {
  auto CondExpr = std::make_unique<ConditionalExpression>();
  CondExpr->Condition = ParseExpression();
  ExpectKeyword("then");
  CondExpr->ThenExpr = ParseExpression();

  if (MatchKeyword("else")) {
    CondExpr->ElseExpr = ParseExpression();
  }

  ExpectKeyword("end");
  if (MatchKeyword("if")) {
    // Explicit end if
  }
  Expect(TokKind::Semicolon);

  return CondExpr;
}

// Statements
std::unique_ptr<AssignmentStatement> AdaParser::ParseAssignmentStatement() {
  auto Assignment = std::make_unique<AssignmentStatement>();
  Assignment->Target = ParseIdentifier();
  Expect(TokKind::Colon);
  Expect(TokKind::Equals);
  Assignment->Value = ParseExpression();
  Expect(TokKind::Semicolon);
  return Assignment;
}

std::unique_ptr<IfStatement> AdaParser::ParseIfStatement() {
  auto If = std::make_unique<IfStatement>();
  If->Condition = ParseExpression();
  ExpectKeyword("then");
  If->ThenStatements = ParseStatements();

  // Parse elsif/else
  while (MatchKeyword("elsif")) {
    auto ElsifCondition = ParseExpression();
    ExpectKeyword("then");
    auto ElsifStatements = ParseStatements();
    If->Elsifs.push_back(std::make_pair(std::move(ElsifCondition),
                                        std::move(ElsifStatements)));
  }

  if (MatchKeyword("else")) {
    If->ElseStatements = ParseStatements();
  }

  ExpectKeyword("end");
  if (MatchKeyword("if")) {
    // Explicit end if
  }
  Expect(TokKind::Semicolon);

  return If;
}

std::unique_ptr<LoopStatement> AdaParser::ParseLoopStatement() {
  auto Loop = std::make_unique<LoopStatement>();
  Loop->Body = ParseStatements();
  ExpectKeyword("end");
  if (MatchKeyword("loop")) {
    // Explicit end loop
  }
  Expect(TokKind::Semicolon);
  return Loop;
}

void AdaParser::ParseExceptionHandlers(ExceptionHandlerList &Handlers) {
  // Repeated: when <Name | others> is <statements>
  while (MatchKeyword("when")) {
    std::string Name;
    if (CurrentToken.Kind == TokKind::Identifier) {
      Name = CurrentToken.Spelling;
      Advance();
    } else if (MatchKeyword("others")) {
      Name = "others";
    }
    ExpectKeyword("is");
    std::vector<std::unique_ptr<StatementNode>> HandlerStmts =
        ParseStatements();
    Handlers.emplace_back(std::move(Name), std::move(HandlerStmts));
  }
}

std::unique_ptr<TryStatement> AdaParser::ParseTryStatement() {
  auto Try = std::make_unique<TryStatement>();
  Try->Body = ParseStatements();
  ExpectKeyword("exception");
  ParseExceptionHandlers(Try->Handlers);
  ExpectKeyword("end");
  if (MatchKeyword("try")) {
    // Explicit end try
  }
  Expect(TokKind::Semicolon);
  return Try;
}

std::unique_ptr<ForLoopStatement> AdaParser::ParseForLoopStatement() {
  auto ForLoop = std::make_unique<ForLoopStatement>();

  // For loop variable
  if (CurrentToken.Kind == TokKind::Identifier) {
    ForLoop->Variable = CurrentToken.Spelling;
    Advance();
  }

  ExpectKeyword("in");

  // Check for reverse
  if (MatchKeyword("reverse")) {
    ForLoop->IsReverse = true;
  }

  // Range
  ForLoop->Low = ParseExpression();
  Expect(TokKind::DotDot);
  ForLoop->High = ParseExpression();

  ExpectKeyword("loop");

  // Loop body
  ForLoop->Body = ParseStatements();

  ExpectKeyword("end");
  if (MatchKeyword("loop")) {
    // Explicit end loop
  }
  Expect(TokKind::Semicolon);

  return ForLoop;
}

std::unique_ptr<WhileLoopStatement> AdaParser::ParseWhileLoopStatement() {
  auto WhileLoop = std::make_unique<WhileLoopStatement>();
  WhileLoop->Condition = ParseExpression();
  ExpectKeyword("loop");
  WhileLoop->Body = ParseStatements();
  ExpectKeyword("end");
  if (MatchKeyword("loop")) {
    // Explicit end loop
  }
  Expect(TokKind::Semicolon);
  return WhileLoop;
}

std::unique_ptr<CaseStatement> AdaParser::ParseCaseStatement() {
  auto Case = std::make_unique<CaseStatement>();
  Case->Discriminant = ParseExpression();
  ExpectKeyword("is");

  // Parse alternatives
  while (!AtEnd() && !MatchKeyword("end")) {
    // Match "when" keyword
    if (!MatchKeyword("when")) {
      break;
    }

    // Parse choices
    std::vector<std::unique_ptr<ExpressionNode>> Choices;
    // Check for "others"
    if (MatchKeyword("others")) {
      // "others" is a special choice that matches everything
    } else {
      while (true) {
        auto Choice = ParseExpression();
        if (Choice) {
          Choices.push_back(std::move(Choice));
        }
        if (!Match(TokKind::Comma)) {
          break;
        }
      }
    }

    Expect(TokKind::Arrow);
    auto Statements = ParseStatements();
    Case->Alternatives.push_back(std::make_pair(std::move(Choices),
                                                std::move(Statements)));
    Expect(TokKind::Semicolon);
  }

  ExpectKeyword("end");
  if (MatchKeyword("case")) {
    // Explicit end case
  }
  Expect(TokKind::Semicolon);

  return Case;
}

std::unique_ptr<BlockStatement> AdaParser::ParseBlockStatement() {
  auto Block = std::make_unique<BlockStatement>();

  // Optional label
  if (CurrentToken.Kind == TokKind::Identifier) {
    Block->Label = CurrentToken.Spelling;
    Advance();
    Expect(TokKind::Colon);
  }

  // Declarations
  Block->Declarations = ParseDeclarations();

  // Statements
  if (MatchKeyword("begin")) {
    Block->Statements = ParseStatements();
  }

  ExpectKeyword("end");
  if (Block->Label.empty()) {
    if (MatchKeyword("block")) {
      // Explicit end block
    }
  } else {
    if (CurrentToken.Kind == TokKind::Identifier &&
        CurrentToken.Spelling == Block->Label) {
      Advance();
    }
  }
  Expect(TokKind::Semicolon);

  return Block;
}

std::unique_ptr<ProcedureCallStatement>
AdaParser::ParseProcedureCallStatement(std::unique_ptr<ExpressionNode> Prefix) {
  auto Call = std::make_unique<ProcedureCallStatement>();
  Call->Procedure = Prefix ? std::move(Prefix) : ParseIdentifier();

  // Check for arguments
  if (Match(TokKind::LParen)) {
    if (!Match(TokKind::RParen)) {
      do {
        if (CurrentToken.Kind == TokKind::Identifier &&
            PeekToken.Kind == TokKind::Colon) {
          // Named argument
          std::string ArgName = CurrentToken.Spelling;
          Advance();
          Match(TokKind::Colon);
          auto Arg = ParseExpression();
          Call->NamedArguments.push_back(ArgName);
        } else {
          auto Arg = ParseExpression();
        }
      } while (Match(TokKind::Comma));
      Expect(TokKind::RParen);
    }
  }

  Expect(TokKind::Semicolon);
  return Call;
}

std::unique_ptr<ReturnStatement> AdaParser::ParseReturnStatement() {
  auto Return = std::make_unique<ReturnStatement>();

  // Optional return value
  if (!AtEnd() && !Match(TokKind::Semicolon)) {
    Return->Value = ParseExpression();
  }

  Expect(TokKind::Semicolon);
  return Return;
}

// Aspect/pragma parsing
std::unique_ptr<AspectSpecification> AdaParser::ParseAspectSpecification() {
  auto Aspect = std::make_unique<AspectSpecification>();

  if (CurrentToken.Kind != TokKind::Identifier) {
    std::cerr << "AdaParser: expected aspect name at line "
              << CurrentToken.Line << "\n";
    return Aspect;
  }
  Aspect->AspectName = CurrentToken.Spelling;
  Advance();

  // Check for value (Aspect_Name => Value)
  if (Match(TokKind::Arrow)) {
    Aspect->AspectValue = ParseExpression();
  }

  return Aspect;
}

std::vector<std::unique_ptr<AspectSpecification>> AdaParser::ParseAspects() {
  std::vector<std::unique_ptr<AspectSpecification>> Aspects;

  while (!AtEnd()) {
    auto Aspect = ParseAspectSpecification();
    if (Aspect) {
      Aspects.push_back(std::move(Aspect));
    }
    if (!Match(TokKind::Comma)) {
      break;
    }
  }

  return Aspects;
}

std::unique_ptr<DeclarationNode> AdaParser::ParseGenericInstantiation(
    bool IsFunction, const std::string &Name) {
  auto Inst = std::make_unique<GenericInstantiation>();
  Inst->Name = Name;

  // Parse the generic name
  if (CurrentToken.Kind == TokKind::Identifier) {
    Inst->GenericName = CurrentToken.Spelling;
    Advance();
  }

  // Parse the formal parameter associations
  Expect(TokKind::LParen);
  if (!Match(TokKind::RParen)) {
    do {
      // Parse association: Name => Expression
      if (CurrentToken.Kind == TokKind::Identifier) {
        std::string FormalName = CurrentToken.Spelling;
        Advance();
        Expect(TokKind::Arrow);
        auto Expr = ParseExpression();
        Inst->Actuals[FormalName] = std::move(Expr);
      }
    } while (Match(TokKind::Semicolon));
    Expect(TokKind::RParen);
  }

  return Inst;
}

std::unique_ptr<TaskDeclaration> AdaParser::ParseTaskDeclaration() {
  auto Task = std::make_unique<TaskDeclaration>();

  // Task name
  if (CurrentToken.Kind != TokKind::Identifier) {
    std::cerr << "AdaParser: expected task name at line "
              << CurrentToken.Line << "\n";
    return Task;
  }
  Task->Name = CurrentToken.Spelling;
  Advance();

  // Check for "is" keyword
  if (MatchKeyword("is")) {
    // Parse entries
    while (!AtEnd()) {
      if (MatchKeyword("entry")) {
        auto Entry = std::make_unique<EntryDeclaration>();
        if (CurrentToken.Kind == TokKind::Identifier) {
          Entry->Name = CurrentToken.Spelling;
          Advance();
        }
        if (Match(TokKind::LParen)) {
          if (!Match(TokKind::RParen)) {
            do {
              auto Params = ParseFormalParameters();
              for (auto &Param : Params) {
                Entry->FormalParameters.push_back(std::move(Param));
              }
            } while (Match(TokKind::Comma));
            Expect(TokKind::RParen);
          }
        }
        Expect(TokKind::Semicolon);
        Task->Entries.push_back(std::move(Entry));
      } else if (MatchKeyword("begin")) {
        // Task body follows immediately
        Task->IsBody = true;
        break;
      } else if (MatchKeyword("end")) {
        break;
      } else {
        break;
      }
    }

    // Expect "end"
    ExpectKeyword("end");

    // Optional task name
    if (CurrentToken.Kind == TokKind::Identifier) {
      Advance();
    }

    // Expect semicolon
    Expect(TokKind::Semicolon);
  } else {
    // Task spec without body - expect semicolon
    Expect(TokKind::Semicolon);
  }

  return Task;
}

std::unique_ptr<TaskBody> AdaParser::ParseTaskBody() {
  auto Body = std::make_unique<TaskBody>();

  // Task name
  if (CurrentToken.Kind != TokKind::Identifier) {
    std::cerr << "AdaParser: expected task name at line "
              << CurrentToken.Line << "\n";
    return Body;
  }
  Body->Name = CurrentToken.Spelling;
  Advance();

  // Parse declarations
  if (MatchKeyword("is")) {
    Body->Declarations = ParseDeclarations();
  }

  // Parse body
  if (MatchKeyword("begin")) {
    Body->BodyStatements = ParseStatements();
  }

  // Expect "end"
  ExpectKeyword("end");

  // Optional task name
  if (CurrentToken.Kind == TokKind::Identifier) {
    Advance();
  }

  // Expect semicolon
  Expect(TokKind::Semicolon);

  return Body;
}

std::unique_ptr<StatementNode> AdaParser::ParseAcceptStatement() {
  auto Stmt = std::make_unique<AcceptStatement>();

  // Entry name
  if (CurrentToken.Kind != TokKind::Identifier) {
    std::cerr << "AdaParser: expected entry name at line "
              << CurrentToken.Line << "\n";
    return Stmt;
  }
  Stmt->EntryName = CurrentToken.Spelling;
  Advance();

  // Parse parameters
  if (Match(TokKind::LParen)) {
    if (!Match(TokKind::RParen)) {
      do {
        auto Params = ParseFormalParameters();
        for (auto &Param : Params) {
          Stmt->FormalParameters.push_back(std::move(Param));
        }
      } while (Match(TokKind::Comma));
      Expect(TokKind::RParen);
    }
  }

  // Parse body
  if (MatchKeyword("do")) {
    Stmt->Body = ParseStatements();
  }

  // Expect "end"
  ExpectKeyword("end");

  // Optional entry name
  if (CurrentToken.Kind == TokKind::Identifier) {
    Advance();
  }

  // Expect semicolon
  Expect(TokKind::Semicolon);

  return Stmt;
}

std::unique_ptr<PragmaDeclaration> AdaParser::ParsePragma() {
  auto Pragma = std::make_unique<PragmaDeclaration>();

  if (CurrentToken.Kind != TokKind::Identifier) {
    std::cerr << "AdaParser: expected pragma name at line "
              << CurrentToken.Line << "\n";
    return Pragma;
  }
  Pragma->PragmaName = CurrentToken.Spelling;
  Advance();

  // Parse pragma arguments
  if (Match(TokKind::LParen)) {
    if (!Match(TokKind::RParen)) {
      do {
        auto Arg = ParseExpression();
        if (Arg) {
          Pragma->Arguments.push_back(std::move(Arg));
        }
      } while (Match(TokKind::Comma));
      Expect(TokKind::RParen);
    }
  }

  Expect(TokKind::Semicolon);
  return Pragma;
}

// Utility
bool AdaParser::IsTypeStart() {
  if (AtEnd()) return false;
  if (CurrentToken.Kind == TokKind::Keyword) {
    return CurrentToken.Text == "integer" || CurrentToken.Text == "natural" ||
           CurrentToken.Text == "positive" || CurrentToken.Text == "float" ||
           CurrentToken.Text == "short_float" || CurrentToken.Text == "long_float" ||
           CurrentToken.Text == "character" || CurrentToken.Text == "wide_character" ||
           CurrentToken.Text == "wide_wide_character" || CurrentToken.Text == "boolean" ||
           CurrentToken.Text == "string" || CurrentToken.Text == "array" ||
           CurrentToken.Text == "record" || CurrentToken.Text == "access";
  }
  if (CurrentToken.Kind == TokKind::Identifier) {
    return true;
  }
  return false;
}
