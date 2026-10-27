//===--- SemanticAnalyzer.h - Ada semantic analysis -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Semantic analysis for Ada 2012: type checking, name resolution,
// overload resolution, expression analysis, aspect/pragma validation.
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_ADA_SEMANTICS_SEMANANALYZER_H
#define CLANG_ADA_SEMANTICS_SEMANANALYZER_H

#include "../AST/AST.h"
#include "../Parser/AdaParser.h"
#include "../Frontend/AdaFrontendAction.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace clang {
namespace ada {

class SemanticAnalyzer {
public:
  SemanticAnalyzer();
  ~SemanticAnalyzer();

  // Analyze a library unit
  bool AnalyzeLibraryUnit(LibraryUnit &Unit);

  // Analyze declarations
  bool AnalyzeDeclarations(const std::vector<std::unique_ptr<DeclarationNode>> &Decls);

  // Analyze statements
  bool AnalyzeStatements(const std::vector<std::unique_ptr<StatementNode>> &Stmts);

  // Analyze expressions
  bool AnalyzeExpression(ExpressionNode *Expr);

  // Type checking
  bool CheckTypeCompatibility(TypeNode *Left, TypeNode *Right);
  bool IsAssignable(TypeNode *Target, TypeNode *Source);

  // Name resolution
  DeclarationNode *ResolveName(const std::string &Name, int Scope);
  bool IsDeclared(const std::string &Name);

  // Overload resolution
  SubprogramDeclaration *ResolveOverload(const std::string &Name,
                                         const std::vector<TypeNode *> &ArgTypes);

  // Freezing
  void FreezeType(TypeNode *Type);

  // Elaboration order analysis (P10)
  bool AnalyzeElaboration(LibraryUnit &Unit);

private:
  bool HasError;

  // Scope stack for name resolution (supports overloads)
  std::vector<std::map<std::string, std::vector<DeclarationNode *>>> ScopeStack;

  // Usage tracking for -Wunused-variable / -Wunused-parameter. One set per
  // enclosing subprogram (lowercased names). An identifier use is recorded in
  // every active set so uses in nested subprograms count for outer ones too.
  std::vector<std::set<std::string>> UsedNamesStack;
  void RecordUse(const std::string &Name);
  bool IsUsed(const std::string &Name) const;

  // "note:" suggestion helper: after an undeclared-identifier error, look for
  // a nearby in-scope name and emit a did-you-mean note.
  void SuggestDeclaration(const std::string &Name);

  // Tagged-type registry (RM 3.9.2). Keys are lowercased; TaggedParent maps
  // each tagged type to its (lowercased) parent name, "" for a root.
  std::map<std::string, std::string> TaggedParent;
  std::set<std::string> TaggedTypes;

  // Elaboration order tracking (P10)
  std::vector<std::string> ElaborationOrder;
  std::set<std::string> ElaboratedPackages;
  std::map<std::string, std::set<std::string>> ElaborateDependencies;

  // Tagged-type helpers
  static std::string ToLowerName(const std::string &Name);
  bool IsTaggedTypeName(const std::string &Name) const;
  std::string FindTaggedParent(const std::string &Name) const;
  bool IsTaggedAncestorOrSame(const std::string &Formal,
                              const std::string &Actual) const;
  int TaggedDepth(const std::string &Name) const;
  int TaggedDepthOfFormal(SubprogramDeclaration *Sub) const;
  static std::string GetTypeName(TypeNode *Type);

  // Analysis helpers
  bool AnalyzeSubprogramDeclaration(SubprogramDeclaration *Decl);
  bool AnalyzeObjectDeclaration(ObjectDeclaration *Decl);
  bool AnalyzeTypeDeclaration(TypeDeclaration *Decl);
  bool AnalyzePackageDeclaration(PackageDeclaration *Decl);
  bool AnalyzePragma(PragmaDeclaration *Decl);
  bool ValidatePragmaName(const std::string &Name);
  bool AnalyzeAspectSpecification(AspectSpecification *Aspect);

  bool AnalyzeAssignmentStatement(AssignmentStatement *Stmt);
  bool AnalyzeIfStatement(IfStatement *Stmt);
  bool AnalyzeLoopStatement(LoopStatement *Stmt);
  bool AnalyzeForLoopStatement(ForLoopStatement *Stmt);
  bool AnalyzeWhileLoopStatement(WhileLoopStatement *Stmt);
  bool AnalyzeCaseStatement(CaseStatement *Stmt);

  bool AnalyzeBinaryOperator(BinaryExpression *Expr);
  bool AnalyzeUnaryOperator(UnaryExpression *Expr);
  bool AnalyzeSubprogramCall(CallExpression *Expr);
  bool AnalyzeAggregate(AggregateExpression *Expr);

  // Type inference
  TypeNode *InferType(ExpressionNode *Expr);
};

} // namespace ada
} // namespace clang

#endif // CLANG_ADA_SEMANTICS_SEMANANALYZER_H