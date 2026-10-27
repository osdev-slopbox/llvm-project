//===--- AdaParser.h - Ada 2012 parser --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Ada 2012 recursive descent parser for declaration items.
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_ADA_PARSER_ADAPARSER_H
#define CLANG_ADA_PARSER_ADAPARSER_H

#include "../Common/Token.h"
#include "../AST/AST.h"
#include "../Lexer/AdaLexer.h"
#include <vector>
#include <string>
#include <memory>

namespace clang {
namespace ada {

class AdaLexer;

// Library unit wrapper
struct LibraryUnit {
  std::string Name;
  std::unique_ptr<ASTNode> Root;
  std::vector<std::string> WithClauses;  // Package names imported via 'with'

  LibraryUnit() = default;
  LibraryUnit(const LibraryUnit&) = delete;
  LibraryUnit& operator=(const LibraryUnit&) = delete;
  LibraryUnit(LibraryUnit&&) = default;
  LibraryUnit& operator=(LibraryUnit&&) = default;
};

class AdaParser {
public:
  AdaParser(const std::string &FileName, const std::string &Source);

  // Parse library unit. Returns null on error.
  std::unique_ptr<LibraryUnit> ParseLibraryUnit();

  // Parse compilation unit (spec or body)
  std::unique_ptr<LibraryUnit> ParseCompilationUnit();

  // Get list of parsed library units
  const std::vector<LibraryUnit>& GetLibraryUnits() const { return Units; }

  // Dump the AST
  void DumpAST();

private:
  std::string FileName;
  std::string Source;
  std::vector<LibraryUnit> Units;

  std::unique_ptr<AdaLexer> Lexer;
  Token CurrentToken;
  Token PeekToken;

  void Advance();
  bool Match(TokKind Kind);
  bool MatchKeyword(const std::string &Keyword);
  Token Expect(TokKind Kind);
  Token ExpectKeyword(const std::string &Keyword);
  bool AtEnd();

  // Declaration parsing
  std::unique_ptr<PackageDeclaration> ParsePackageDeclaration();
  std::unique_ptr<SubprogramDeclaration> ParseSubprogramDeclaration(bool IsFunction,
      const std::string &Name = "");
  std::unique_ptr<ObjectDeclaration> ParseObjectDeclaration();
  std::unique_ptr<TypeDeclaration> ParseTypeDeclaration();
  std::vector<std::unique_ptr<FormalParameter>> ParseFormalParameters();
  std::vector<std::unique_ptr<DeclarationNode>> ParseDeclarations();
  std::unique_ptr<DeclarationNode> ParseDeclaration();
  std::unique_ptr<DeclarationNode> ParseGenericInstantiation(bool IsFunction, const std::string &Name);
  std::unique_ptr<TaskDeclaration> ParseTaskDeclaration();
  std::unique_ptr<TaskBody> ParseTaskBody();
  std::unique_ptr<StatementNode> ParseAcceptStatement();

  // Type parsing
  std::unique_ptr<TypeNode> ParseType();
  bool ParseTaggedHead(TypeDeclaration &TypeDecl);
  std::unique_ptr<ScalarType> ParseScalarType();
  std::unique_ptr<TypeNode> ParseCompositeType();

  // Expression parsing
  std::unique_ptr<ExpressionNode> ParseExpression();
  std::unique_ptr<ExpressionNode> ParseRelationalExpression();
  std::unique_ptr<ExpressionNode> ParseSimpleExpression();
  std::unique_ptr<ExpressionNode> ParseTerm();
  std::unique_ptr<ExpressionNode> ParseFactor();
  std::unique_ptr<ExpressionNode> ParsePrimary();
  std::unique_ptr<ExpressionNode> ParseNamedAggregate();
  std::unique_ptr<ExpressionNode> ParseLiteral();
  std::unique_ptr<ExpressionNode> ParseIdentifier();

  // Statement parsing
  std::vector<std::unique_ptr<StatementNode>> ParseStatements();
  std::unique_ptr<StatementNode> ParseStatement();
  std::unique_ptr<AssignmentStatement> ParseAssignmentStatement();
  std::unique_ptr<IfStatement> ParseIfStatement();
  std::unique_ptr<LoopStatement> ParseLoopStatement();
  std::unique_ptr<ForLoopStatement> ParseForLoopStatement();
  std::unique_ptr<WhileLoopStatement> ParseWhileLoopStatement();
  std::unique_ptr<TryStatement> ParseTryStatement();
  void ParseExceptionHandlers(ExceptionHandlerList &Handlers);
  std::unique_ptr<CaseStatement> ParseCaseStatement();
  std::unique_ptr<BlockStatement> ParseBlockStatement();
  std::unique_ptr<ProcedureCallStatement>
  ParseProcedureCallStatement(std::unique_ptr<ExpressionNode> Prefix = nullptr);
  std::unique_ptr<ReturnStatement> ParseReturnStatement();

  // Expression statement forms
  std::unique_ptr<ExpressionNode> ParseCaseExpression();
  std::unique_ptr<ExpressionNode> ParseConditionalExpression();

  // Aspect/pragma parsing
  std::unique_ptr<AspectSpecification> ParseAspectSpecification();
  std::vector<std::unique_ptr<AspectSpecification>> ParseAspects();
  std::unique_ptr<PragmaDeclaration> ParsePragma();

  // Utility
  bool IsDeclarationStart();
  bool IsStatementStart();
  bool IsTypeStart();
};

} // namespace ada
} // namespace clang

#endif // CLANG_ADA_PARSER_ADAPARSER_H
