//===--- SemanticAnalyzer.cpp - Ada semantic analysis ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/Semantics/SemanticAnalyzer.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

using namespace clang;
using namespace clang::ada;

// Defined below; used by overload resolution and name lookup.
static bool IsRuntimeBuiltin(const std::string &Name);

SemanticAnalyzer::SemanticAnalyzer() : HasError(false) {
  // Push global scope
  ScopeStack.push_back(std::map<std::string, std::vector<DeclarationNode *>>());
}

SemanticAnalyzer::~SemanticAnalyzer() {}

bool SemanticAnalyzer::AnalyzeLibraryUnit(LibraryUnit &Unit) {
  if (!Unit.Root) {
    return true; // Nothing to analyze
  }

  // Push unit scope
  ScopeStack.push_back(std::map<std::string, std::vector<DeclarationNode *>>());

  // Register with-clause imports (package names become visible)
  for (const auto &PkgName : Unit.WithClauses) {
    ScopeStack.back()[PkgName].push_back(nullptr);
  }

  // Register unit name
  if (!Unit.Name.empty()) {
    ScopeStack.back()[Unit.Name].push_back(static_cast<DeclarationNode *>(Unit.Root.get()));
  }

  bool Result = true;

  // Analyze based on declaration kind
  if (Unit.Root->getKind() == NodeKind::SubprogramDeclaration) {
    Result = AnalyzeSubprogramDeclaration(
        static_cast<SubprogramDeclaration *>(Unit.Root.get()));
  } else if (Unit.Root->getKind() == NodeKind::PackageDeclaration) {
    Result = AnalyzePackageDeclaration(
        static_cast<PackageDeclaration *>(Unit.Root.get()));
  } else if (Unit.Root->getKind() == NodeKind::TypeDeclaration) {
    Result = AnalyzeTypeDeclaration(
        static_cast<TypeDeclaration *>(Unit.Root.get()));
  } else if (Unit.Root->getKind() == NodeKind::GenericInstantiation) {
    // Generic instantiation: expand by creating a new subprogram declaration
    // with the substituted types (instantiation expansion, RM 12.5)
    auto *Inst = static_cast<GenericInstantiation *>(Unit.Root.get());
    
    // For P4, we just register the instantiated name in scope.
    // The actual instantiation expansion (creating a copy of the generic
    // body with substituted types) will be done in codegen.
    ScopeStack.back()[Inst->Name].push_back(static_cast<DeclarationNode *>(Unit.Root.get()));
    Result = true;
  }

  // Pop unit scope
  ScopeStack.pop_back();

  HasError = HasError || !Result;
  return Result;
}

bool SemanticAnalyzer::AnalyzeDeclarations(
    const std::vector<std::unique_ptr<DeclarationNode>> &Decls) {
  bool Result = true;

  for (auto &Decl : Decls) {
    if (!Decl) {
      continue;
    }

    // Register declaration in current scope (check for name based on kind)
    switch (Decl->getKind()) {
    case NodeKind::SubprogramDeclaration: {
      auto *SubDecl = static_cast<SubprogramDeclaration *>(Decl.get());
      if (!SubDecl->Name.empty()) {
        ScopeStack.back()[SubDecl->Name].push_back(Decl.get());
      }
      Result = Result && AnalyzeSubprogramDeclaration(SubDecl);
      break;
    }
    case NodeKind::ObjectDeclaration: {
      auto *ObjDecl = static_cast<ObjectDeclaration *>(Decl.get());
      // Register all declarator names (handles X, Y : T)
      if (!ObjDecl->Names.empty()) {
        for (const auto &Name : ObjDecl->Names) {
          ScopeStack.back()[Name].push_back(Decl.get());
        }
      } else if (!ObjDecl->Name.empty()) {
        ScopeStack.back()[ObjDecl->Name].push_back(Decl.get());
      }
      Result = Result && AnalyzeObjectDeclaration(ObjDecl);
      break;
    }
    case NodeKind::TypeDeclaration: {
      auto *TypeDecl = static_cast<TypeDeclaration *>(Decl.get());
      if (!TypeDecl->Name.empty()) {
        ScopeStack.back()[TypeDecl->Name].push_back(Decl.get());
      }
      Result = Result && AnalyzeTypeDeclaration(TypeDecl);
      break;
    }
    case NodeKind::PragmaDeclaration:
      Result = Result && AnalyzePragma(static_cast<PragmaDeclaration *>(Decl.get()));
      break;
    default:
      break;
    }
  }

  return Result;
}

bool SemanticAnalyzer::AnalyzeStatements(
    const std::vector<std::unique_ptr<StatementNode>> &Stmts) {
  bool Result = true;

  for (auto &Stmt : Stmts) {
    if (!Stmt) {
      continue;
    }

    // Analyze based on statement kind
    switch (Stmt->getKind()) {
    case NodeKind::AssignmentStatement:
      Result = Result && AnalyzeAssignmentStatement(
                             static_cast<AssignmentStatement *>(Stmt.get()));
      break;
    case NodeKind::IfStatement:
      Result = Result && AnalyzeIfStatement(
                             static_cast<IfStatement *>(Stmt.get()));
      break;
    case NodeKind::LoopStatement:
      Result = Result && AnalyzeLoopStatement(
                             static_cast<LoopStatement *>(Stmt.get()));
      break;
    case NodeKind::ForLoopStatement:
      Result = Result && AnalyzeForLoopStatement(
                             static_cast<ForLoopStatement *>(Stmt.get()));
      break;
    case NodeKind::WhileLoopStatement:
      Result = Result && AnalyzeWhileLoopStatement(
                             static_cast<WhileLoopStatement *>(Stmt.get()));
      break;
    case NodeKind::CaseStatement:
      Result = Result && AnalyzeCaseStatement(
                             static_cast<CaseStatement *>(Stmt.get()));
      break;
    case NodeKind::PragmaStatement:
      Result = Result && ValidatePragmaName(
                             static_cast<PragmaStatement *>(Stmt.get())->PragmaName);
      break;
    case NodeKind::ProcedureCallStatement: {
      // Analyze the callee name and each actual parameter so that identifier
      // uses are recorded and undeclared names are diagnosed. Overload
      // resolution is deliberately left to the code generator here: binding
      // the call statically would defeat dynamic dispatch on tagged types.
      auto *Call = static_cast<ProcedureCallStatement *>(Stmt.get());
      if (Call->Procedure) {
        if (Call->Procedure->getKind() == NodeKind::CallExpression) {
          auto *CE = static_cast<CallExpression *>(Call->Procedure.get());
          if (CE->Function)
            Result = Result && AnalyzeExpression(CE->Function.get());
          for (auto &Arg : CE->Arguments)
            Result = Result && AnalyzeExpression(Arg.get());
        } else {
          Result = Result && AnalyzeExpression(Call->Procedure.get());
          for (auto &Arg : Call->Arguments)
            Result = Result && AnalyzeExpression(Arg.get());
        }
      }
      break;
    }
    default:
      break;
    }
  }

  return Result;
}

bool SemanticAnalyzer::AnalyzeExpression(ExpressionNode *Expr) {
  if (!Expr) {
    return true;
  }

  bool Result = true;

  // Analyze based on expression kind
  switch (Expr->getKind()) {
  case NodeKind::BinaryExpression:
    Result = AnalyzeBinaryOperator(static_cast<BinaryExpression *>(Expr));
    break;
  case NodeKind::UnaryExpression:
    Result = AnalyzeUnaryOperator(static_cast<UnaryExpression *>(Expr));
    break;
  case NodeKind::CallExpression:
    Result = AnalyzeSubprogramCall(static_cast<CallExpression *>(Expr));
    break;
  case NodeKind::AggregateExpression:
    Result = AnalyzeAggregate(static_cast<AggregateExpression *>(Expr));
    break;
  case NodeKind::TypeConversionExpression: {
    auto Conversion = static_cast<TypeConversionExpression *>(Expr);
    // Analyze the operand
    Result = AnalyzeExpression(Conversion->Operand.get());
    break;
  }
  case NodeKind::IdentifierExpression: {
    auto Id = static_cast<IdentifierExpression *>(Expr);
    // Record usage for -Wunused-variable / -Wunused-parameter.
    RecordUse(Id->Name);
    // 'Result is a predefined attribute name that refers to the function
    // result (RM 5.5.1(16)). It is always available in a function body,
    // even though it is not declared as a variable.
    if (Id->Name != "Result" && !IsDeclared(Id->Name)) {
      GetDiagnostics().Diag("", DiagSeverity::Error,
                            "undeclared identifier: " + Id->Name);
      SuggestDeclaration(Id->Name);
      return false;
    }
    break;
  }
  default:
    break;
  }

  // Infer type for this expression
  InferType(Expr);

  return Result;
}

// Levenshtein edit distance (used for "did you mean" suggestions).
static int EditDistance(const std::string &A, const std::string &B) {
  const size_t N = A.size(), M = B.size();
  std::vector<int> Prev(M + 1), Cur(M + 1);
  for (size_t j = 0; j <= M; ++j)
    Prev[j] = static_cast<int>(j);
  for (size_t i = 1; i <= N; ++i) {
    Cur[0] = static_cast<int>(i);
    for (size_t j = 1; j <= M; ++j) {
      int Cost = (A[i - 1] == B[j - 1]) ? 0 : 1;
      int Del = Prev[j] + 1;
      int Ins = Cur[j - 1] + 1;
      int Sub = Prev[j - 1] + Cost;
      Cur[j] = std::min(Del, std::min(Ins, Sub));
    }
    Prev.swap(Cur);
  }
  return Prev[M];
}

void SemanticAnalyzer::SuggestDeclaration(const std::string &Name) {
  const std::string Lower = ToLowerName(Name);
  std::string Best;
  int BestDist = 3; // only suggest close matches
  for (const auto &Scope : ScopeStack) {
    for (const auto &Entry : Scope) {
      int Dist = EditDistance(Lower, ToLowerName(Entry.first));
      if (Dist < BestDist) {
        BestDist = Dist;
        Best = Entry.first;
      }
    }
  }
  if (!Best.empty())
    GetDiagnostics().Note("", "did you mean '" + Best + "'?");
  else
    GetDiagnostics().Note("", "did you mean to declare '" + Name + "'?");
}

void SemanticAnalyzer::RecordUse(const std::string &Name) {
  const std::string Lower = ToLowerName(Name);
  for (auto &Set : UsedNamesStack)
    Set.insert(Lower);
}

bool SemanticAnalyzer::IsUsed(const std::string &Name) const {
  return !UsedNamesStack.empty() &&
         UsedNamesStack.back().count(ToLowerName(Name)) != 0;
}

bool SemanticAnalyzer::AnalyzeSubprogramDeclaration(
    SubprogramDeclaration *Decl) {
  if (!Decl) {
    return true;
  }

  bool Result = true;

  // Push subprogram scope
  ScopeStack.push_back(std::map<std::string, std::vector<DeclarationNode *>>());
  UsedNamesStack.push_back(std::set<std::string>());

  // Register parameters in scope
  for (auto &Param : Decl->FormalParameters) {
    if (Param && !Param->Name.empty()) {
      ScopeStack.back()[Param->Name].push_back(Param.get());
    }
  }

  // Analyze body declarations if present
  if (!Decl->Declarations.empty()) {
    Result = Result && AnalyzeDeclarations(Decl->Declarations);
  }

  // Analyze body statements if present
  if (!Decl->BodyStatements.empty()) {
    Result = Result && AnalyzeStatements(Decl->BodyStatements);
  }

  // Analyze aspect specifications
  for (auto &Aspect : Decl->Aspects) {
    if (Aspect) {
      Result = Result && AnalyzeAspectSpecification(Aspect.get());
    }
  }

  // -Wunused-parameter / -Wunused-variable diagnostics (RM 6.1, 3.3).
  // Parameters named "_" or starting with "_" are exempt (RM 6.1(22)).
  for (auto &Param : Decl->FormalParameters) {
    if (!Param || Param->Name.empty() || Param->Name[0] == '_')
      continue;
    if (!IsUsed(Param->Name))
      GetDiagnostics().Diag("", DiagSeverity::Warning,
                            "unused parameter '" + Param->Name + "'",
                            WarningKind::UnusedParameter);
  }
  for (auto &D : Decl->Declarations) {
    if (!D || D->getKind() != NodeKind::ObjectDeclaration)
      continue;
    auto *Obj = static_cast<ObjectDeclaration *>(D.get());
    if (Obj->Name.empty() || Obj->Name[0] == '_')
      continue;
    if (!IsUsed(Obj->Name))
      GetDiagnostics().Diag("", DiagSeverity::Warning,
                            "unused variable '" + Obj->Name + "'",
                            WarningKind::UnusedVariable);
  }

  // Pop subprogram scope
  ScopeStack.pop_back();
  UsedNamesStack.pop_back();

  return Result;
}

bool SemanticAnalyzer::AnalyzeObjectDeclaration(ObjectDeclaration *Decl) {
  if (!Decl) {
    return true;
  }

  bool Result = true;

  // If initializer present, analyze it
  if (Decl->Initializer) {
    Result = AnalyzeExpression(Decl->Initializer.get());
  }

  // Freeze the type when an object of that type is declared
  if (Decl->Type) {
    FreezeType(Decl->Type.get());
  }

  // Analyze aspect specifications
  for (auto &Aspect : Decl->Aspects) {
    if (Aspect) {
      Result = Result && AnalyzeAspectSpecification(Aspect.get());
    }
  }

  return Result;
}

bool SemanticAnalyzer::AnalyzeTypeDeclaration(TypeDeclaration *Decl) {
  if (!Decl) {
    return true;
  }

  // Register tagged types (RM 3.9.2). A derived type is tagged only when
  // its parent is tagged; otherwise the 'tagged' aspect is silently dropped
  // in P3 (a proper diagnostic comes later).
  if (Decl->IsTagged) {
    if (!Decl->Parent.empty() && !IsTaggedTypeName(Decl->Parent)) {
      Decl->IsTagged = false;
    } else {
      TaggedTypes.insert(ToLowerName(Decl->Name));
      TaggedParent[ToLowerName(Decl->Name)] = ToLowerName(Decl->Parent);
    }
  }

  // Analyze aspect specifications
  for (auto &Aspect : Decl->Aspects) {
    if (Aspect) {
      if (!AnalyzeAspectSpecification(Aspect.get())) {
        return false;
      }
    }
  }

  return true;
}

bool SemanticAnalyzer::AnalyzePackageDeclaration(PackageDeclaration *Decl) {
  if (!Decl) {
    return true;
  }

  bool Result = true;

  // Push package scope
  ScopeStack.push_back(std::map<std::string, std::vector<DeclarationNode *>>());

  // Analyze declarations
  Result = Result && AnalyzeDeclarations(Decl->Declarations);

  // Pop package scope
  ScopeStack.pop_back();

  return Result;
}

bool SemanticAnalyzer::ValidatePragmaName(const std::string &Name) {
  // Validate known pragmas (expanded for GNAT compatibility)
  // Core pragmas
  bool KnownPragma = (Name == "Inline" || Name == "Export" ||
      Name == "Import" || Name == "Pack" ||
      Name == "Suppress" || Name == "Check" ||
      // Elaboration pragmas (P10)
      Name == "Pure" || Name == "Preelaborated" ||
      Name == "Elaborate" || Name == "Elaborate_All" ||
      // Tasking pragmas
      Name == "Priority" || Name == "Interrupt_Priority" ||
      Name == "Protected" || Name == "Asynchronous" ||
      Name == "Asynchronous_Select" || Name == "Terminated" ||
      // Linkage pragmas
      Name == "Link_Name" || Name == "Convention" ||
      // Storage pragmas
      Name == "At" || Name == "Address" ||
      Name == "Storage_Size" || Name == "Alignment" ||
      // Warnings pragmas
      Name == "Warnings" || Name == "Style_Checks" ||
      // Optimization pragmas
      Name == "Optimize" || Name == "Inline_All_Calls" ||
      // Debug pragmas
      Name == "Debug" || Name == "Assert");

  if (!KnownPragma) {
    // Unknown pragma - warn but continue
    GetDiagnostics().Diag("", DiagSeverity::Warning,
                          "unknown pragma: " + Name,
                          WarningKind::UnknownPragma);
  }
  return true;
}

bool SemanticAnalyzer::AnalyzePragma(PragmaDeclaration *Decl) {
  if (!Decl) {
    return true;
  }

  ValidatePragmaName(Decl->PragmaName);

  // Record elaboration dependencies (P10)
  if (Decl->PragmaName == "Elaborate" || Decl->PragmaName == "Elaborate_All") {
    // The first argument is the package name that must be elaborated first
    if (!Decl->Arguments.empty()) {
      if (Decl->Arguments[0]->getKind() == NodeKind::IdentifierExpression) {
        auto *Ident = static_cast<IdentifierExpression *>(Decl->Arguments[0].get());
        // Record dependency - this unit depends on the named package
        ElaborateDependencies["current_unit"].insert(Ident->Name);
      }
    }
  }

  // Analyze pragma arguments
  bool Result = true;
  for (auto &Arg : Decl->Arguments) {
    Result = Result && AnalyzeExpression(Arg.get());
  }

  return Result;
}

bool SemanticAnalyzer::AnalyzeElaboration(LibraryUnit &Unit) {
  // Basic elaboration order validation (P10)
  // In a full implementation, this would:
  // 1. Build an elaboration order graph from with-clauses and Elaborate pragmas
  // 2. Detect circular elaboration dependencies
  // 3. Ensure Preelaborated/Pure packages have no elaboration code
  // 4. Validate that Elaborate/Elaborate_All pragmas are satisfied

  // For now, validate that Elaborate pragmas reference declared packages
  for (const auto &Dep : ElaborateDependencies["current_unit"]) {
    if (!IsDeclared(Dep)) {
      GetDiagnostics().Diag("", DiagSeverity::Error,
                            "elaboration dependency not declared: " + Dep);
      HasError = true;
    }
  }

  return !HasError;
}

bool SemanticAnalyzer::AnalyzeAspectSpecification(AspectSpecification *Aspect) {
  if (!Aspect) {
    return true;
  }

  // Validate known aspect names (expanded for GNAT compatibility)
  // Contract aspects
  bool KnownAspect = (Aspect->AspectName == "Inline" || Aspect->AspectName == "Pre" ||
      Aspect->AspectName == "Post" || Aspect->AspectName == "Invariant" ||
      Aspect->AspectName == "Global" || Aspect->AspectName == "Depends" ||
      // Elaboration aspects (P10)
      Aspect->AspectName == "Pure" || Aspect->AspectName == "Preelaborated" ||
      Aspect->AspectName == "Elaborate" || Aspect->AspectName == "Elaborate_All" ||
      // Linkage aspects
      Aspect->AspectName == "Export" || Aspect->AspectName == "Import" ||
      Aspect->AspectName == "Convention" || Aspect->AspectName == "Link_Name" ||
      // Storage aspects
      Aspect->AspectName == "At" || Aspect->AspectName == "Address" ||
      Aspect->AspectName == "Storage_Size" || Aspect->AspectName == "Alignment" ||
      // Tasking aspects
      Aspect->AspectName == "Priority" || Aspect->AspectName == "Interrupt_Priority" ||
      Aspect->AspectName == "Protected" || Aspect->AspectName == "Asynchronous");

  if (!KnownAspect) {
    // Unknown aspect - warn but continue
    GetDiagnostics().Diag("", DiagSeverity::Warning,
                          "unknown aspect: " + Aspect->AspectName,
                          WarningKind::UnknownAspect);
  }

  // Analyze value if present
  if (Aspect->AspectValue) {
    return AnalyzeExpression(Aspect->AspectValue.get());
  }

  return true;
}

void SemanticAnalyzer::FreezeType(TypeNode *Type) {
  if (Type && !Type->Frozen) {
    Type->Frozen = true;
    // TODO: Report error if type was already frozen (re-freezing)
  }
}

bool SemanticAnalyzer::AnalyzeAssignmentStatement(AssignmentStatement *Stmt) {
  if (!Stmt) {
    return true;
  }

  bool Result = true;

  // Analyze LHS target
  Result = AnalyzeExpression(Stmt->Target.get());

  // Analyze RHS expression
  Result = Result && AnalyzeExpression(Stmt->Value.get());

  // Check type compatibility
  TypeNode *TargetType = InferType(Stmt->Target.get());
  TypeNode *SourceType = InferType(Stmt->Value.get());

  if (TargetType && SourceType && !IsAssignable(TargetType, SourceType)) {
    GetDiagnostics().Diag("", DiagSeverity::Error,
                          "type mismatch in assignment");
    return false;
  }

  return Result;
}

bool SemanticAnalyzer::AnalyzeIfStatement(IfStatement *Stmt) {
  if (!Stmt) {
    return true;
  }

  bool Result = true;

  // Condition must be boolean
  Result = AnalyzeExpression(Stmt->Condition.get());

  // Analyze then branch
  Result = Result && AnalyzeStatements(Stmt->ThenStatements);

  // Analyze elsif branches
  for (auto &Elsif : Stmt->Elsifs) {
    Result = Result && AnalyzeExpression(Elsif.first.get());
    Result = Result && AnalyzeStatements(Elsif.second);
  }

  // Analyze else branch if present
  if (!Stmt->ElseStatements.empty()) {
    Result = Result && AnalyzeStatements(Stmt->ElseStatements);
  }

  return Result;
}

bool SemanticAnalyzer::AnalyzeLoopStatement(LoopStatement *Stmt) {
  if (!Stmt) {
    return true;
  }

  return AnalyzeStatements(Stmt->Body);
}

bool SemanticAnalyzer::AnalyzeForLoopStatement(ForLoopStatement *Stmt) {
  if (!Stmt) {
    return true;
  }

  bool Result = true;

  // Push loop scope for loop variable
  ScopeStack.push_back(std::map<std::string, std::vector<DeclarationNode *>>());

  // Declare loop variable in loop scope
  if (!Stmt->Variable.empty()) {
    ScopeStack.back()[Stmt->Variable].push_back(nullptr);
  }

  // Analyze range expressions
  Result = AnalyzeExpression(Stmt->Low.get());
  Result = Result && AnalyzeExpression(Stmt->High.get());

  // Analyze loop body
  Result = Result && AnalyzeStatements(Stmt->Body);

  // Pop loop scope
  ScopeStack.pop_back();

  return Result;
}

bool SemanticAnalyzer::AnalyzeWhileLoopStatement(WhileLoopStatement *Stmt) {
  if (!Stmt) {
    return true;
  }

  bool Result = true;

  // Condition must be boolean
  Result = AnalyzeExpression(Stmt->Condition.get());

  // Analyze loop body
  Result = Result && AnalyzeStatements(Stmt->Body);

  return Result;
}

bool SemanticAnalyzer::AnalyzeCaseStatement(CaseStatement *Stmt) {
  if (!Stmt) {
    return true;
  }

  bool Result = true;

  // Analyze discriminant
  Result = AnalyzeExpression(Stmt->Discriminant.get());

  // Analyze each alternative
  for (auto &Alt : Stmt->Alternatives) {
    // Analyze choices
    for (auto &Choice : Alt.first) {
      Result = Result && AnalyzeExpression(Choice.get());
    }
    // Analyze statements
    Result = Result && AnalyzeStatements(Alt.second);
  }

  return Result;
}

bool SemanticAnalyzer::AnalyzeBinaryOperator(BinaryExpression *Expr) {
  if (!Expr) {
    return true;
  }

  bool Result = true;

  // Analyze operands
  Result = AnalyzeExpression(Expr->Left.get());
  Result = Result && AnalyzeExpression(Expr->Right.get());

  // TODO: Check operator is valid for operand types

  return Result;
}

bool SemanticAnalyzer::AnalyzeUnaryOperator(UnaryExpression *Expr) {
  if (!Expr) {
    return true;
  }

  return AnalyzeExpression(Expr->Operand.get());
}

bool SemanticAnalyzer::AnalyzeSubprogramCall(CallExpression *Expr) {
  if (!Expr) {
    return true;
  }

  bool Result = true;

  // Analyze the function expression
  Result = AnalyzeExpression(Expr->Function.get());

  // Analyze arguments and infer their types
  std::vector<TypeNode *> ArgTypes;
  for (auto &Arg : Expr->Arguments) {
    Result = Result && AnalyzeExpression(Arg.get());
    TypeNode *ArgType = InferType(Arg.get());
    ArgTypes.push_back(ArgType);
  }

  // Resolve overload
  if (Expr->Function && Expr->Function->getKind() == NodeKind::IdentifierExpression) {
    auto *FuncId = static_cast<IdentifierExpression *>(Expr->Function.get());
    // Runtime-library builtins (Text_IO, Calendar, ...) have no declaration
    // in the AST; the code generator lowers them directly.
    if (IsRuntimeBuiltin(FuncId->Name)) {
      return Result;
    }
    SubprogramDeclaration *Resolved = ResolveOverload(FuncId->Name, ArgTypes);
    if (!Resolved) {
      GetDiagnostics().Diag("", DiagSeverity::Error,
                            "cannot resolve call to " + FuncId->Name);
      return false;
    }
    // Bind the call to the chosen candidate so codegen can dispatch
    // statically (pointer identity) or dynamically on the tag.
    Expr->Resolved = Resolved;
  }

  return Result;
}

bool SemanticAnalyzer::AnalyzeAggregate(AggregateExpression *Expr) {
  if (!Expr) {
    return true;
  }

  bool Result = true;

  // Analyze each element
  for (auto &Elem : Expr->Elements) {
    Result = Result && AnalyzeExpression(Elem.get());
  }

  return Result;
}

bool SemanticAnalyzer::CheckTypeCompatibility(TypeNode *Left, TypeNode *Right) {
  if (!Left || !Right) {
    return true;
  }

  // Different type kinds are not compatible
  if (Left->getKind() != Right->getKind()) {
    return false;
  }

  // For scalar types, check the scalar kind
  if (Left->getKind() == NodeKind::ScalarType) {
    auto *LeftScalar = static_cast<ScalarType *>(Left);
    auto *RightScalar = static_cast<ScalarType *>(Right);
    if (LeftScalar->ScalarKind != RightScalar->ScalarKind) {
      return false;
    }
  }

  // For derived types, check parent type compatibility (RM 4.2)
  if (Left->getKind() == NodeKind::DerivedType) {
    auto *LeftDerived = static_cast<DerivedType *>(Left);
    auto *RightDerived = static_cast<DerivedType *>(Right);
    return LeftDerived->ParentType == RightDerived->ParentType;
  }

  // For access types, check target type compatibility (RM 5.2(4))
  if (Left->getKind() == NodeKind::AccessType) {
    auto *LeftAccess = static_cast<AccessType *>(Left);
    auto *RightAccess = static_cast<AccessType *>(Right);
    if (LeftAccess->TargetType && RightAccess->TargetType) {
      return CheckTypeCompatibility(LeftAccess->TargetType.get(), RightAccess->TargetType.get());
    }
    return !LeftAccess->TargetType && !RightAccess->TargetType;
  }

  // For array types, check element type compatibility (RM 5.2(4))
  if (Left->getKind() == NodeKind::ArrayType) {
    auto *LeftArray = static_cast<ArrayType *>(Left);
    auto *RightArray = static_cast<ArrayType *>(Right);
    if (LeftArray->ElementType && RightArray->ElementType) {
      return CheckTypeCompatibility(LeftArray->ElementType.get(), RightArray->ElementType.get());
    }
    return !LeftArray->ElementType && !RightArray->ElementType;
  }

  return true;
}

bool SemanticAnalyzer::IsAssignable(TypeNode *Target, TypeNode *Source) {
  // For now, same type is assignable
  if (CheckTypeCompatibility(Target, Source)) {
    return true;
  }

  // A value of a descendant tagged type is assignable to an ancestor
  // (RM 5.2(4)/13.3); the reverse requires an explicit conversion.
  std::string TargetName = GetTypeName(Target);
  if (IsTaggedTypeName(TargetName)) {
    std::string Current = GetTypeName(Source);
    int Hops = 0;
    while (!Current.empty() && Hops < 64) {
      if (ToLowerName(Current) == ToLowerName(TargetName)) {
        return true;
      }
      Current = FindTaggedParent(Current);
      ++Hops;
    }
  }

  return false;
}

std::string SemanticAnalyzer::ToLowerName(const std::string &Name) {
  std::string Result = Name;
  for (char &C : Result) {
    if (C >= 'A' && C <= 'Z') {
      C = char(C - 'A' + 'a');
    }
  }
  return Result;
}

bool SemanticAnalyzer::IsTaggedTypeName(const std::string &Name) const {
  return TaggedTypes.count(ToLowerName(Name)) != 0;
}

std::string SemanticAnalyzer::FindTaggedParent(const std::string &Name) const {
  auto Found = TaggedParent.find(ToLowerName(Name));
  return Found == TaggedParent.end() ? std::string() : Found->second;
}

bool SemanticAnalyzer::IsTaggedAncestorOrSame(
    const std::string &Formal, const std::string &Actual) const {
  // RM 8.3(3): a tagged actual parameter conforms to the formal when the
  // actual's type is the same as, or a descendant of, the formal's type.
  // An ancestor-typed actual (e.g. a Shape for a Circle formal) does not.
  // When either name is not a registered tagged type the constraint does
  // not apply; the usual compatibility checks handle it.
  if (Formal.empty() || Actual.empty() || !IsTaggedTypeName(Formal) ||
      !IsTaggedTypeName(Actual)) {
    return true;
  }
  std::string Current = ToLowerName(Actual);
  const std::string Target = ToLowerName(Formal);
  for (int Depth = 0; Depth < 64; ++Depth) {
    if (Current == Target) {
      return true;
    }
    std::string Parent = FindTaggedParent(Current);
    if (Parent.empty()) {
      return false;
    }
    Current = Parent;
  }
  return false;
}

int SemanticAnalyzer::TaggedDepth(const std::string &Name) const {
  int Depth = 0;
  std::string Current = ToLowerName(Name);
  while (!Current.empty() && Depth < 64) {
    auto Found = TaggedParent.find(Current);
    if (Found == TaggedParent.end() || Found->second.empty()) {
      break;
    }
    Current = Found->second;
    ++Depth;
  }
  return Depth;
}

int SemanticAnalyzer::TaggedDepthOfFormal(SubprogramDeclaration *Sub) const {
  if (!Sub || Sub->FormalParameters.empty()) {
    return -1;
  }
  auto &First = Sub->FormalParameters[0];
  if (!First || !First->Type) {
    return -1;
  }
  std::string Name = GetTypeName(First->Type.get());
  if (Name.empty() || !IsTaggedTypeName(Name)) {
    return -1;
  }
  return TaggedDepth(Name);
}

std::string SemanticAnalyzer::GetTypeName(TypeNode *Type) {
  if (!Type) {
    return std::string();
  }
  if (Type->getKind() == NodeKind::ScalarType) {
    return static_cast<ScalarType *>(Type)->Name;
  }
  if (Type->getKind() == NodeKind::DerivedType) {
    return static_cast<DerivedType *>(Type)->ParentType;
  }
  return std::string();
}

DeclarationNode *SemanticAnalyzer::ResolveName(const std::string &Name,
                                                int Scope) {
  // Search from innermost scope outward
  for (size_t i = ScopeStack.size(); i > 0; --i) {
    auto Found = ScopeStack[i - 1].find(Name);
    if (Found != ScopeStack[i - 1].end()) {
      // Return the first declaration (for non-overloaded names)
      if (!Found->second.empty()) {
        return Found->second[0];
      }
    }
  }
  return nullptr;
}

// Names implemented directly by the in-tree runtime and recognized by the
// code generator's GenerateRuntimeCall. Comparison is case-insensitive.
static bool IsRuntimeBuiltin(const std::string &Name) {
  auto Lower = [](std::string S) {
    for (auto &C : S)
      C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
    return S;
  };
  const std::string N = Lower(Name);
  static const char *const Builtins[] = {
      "put",          "put_line",     "new_line",      "print_int",
      "print_float",  "get_int",      "get_float",     "clock",
      "length",       "string_length", "env_value",    "env_exists",
      "environment_value", "environment_exists", "print_int_new_line",
      "print_float_new_line", "value", "exists",
  };
  for (const char *B : Builtins)
    if (N == B)
      return true;
  return false;
}

bool SemanticAnalyzer::IsDeclared(const std::string &Name) {
  // Search from innermost scope outward
  for (size_t i = ScopeStack.size(); i > 0; --i) {
    if (ScopeStack[i - 1].find(Name) != ScopeStack[i - 1].end()) {
      return true;
    }
  }

  // Runtime-library builtins are always visible (Ada.Text_IO,
  // Ada.Integer_Text_IO, Ada.Float_Text_IO, Ada.Calendar, Ada.Strings,
  // Ada.Environment_Variables minima implemented in the in-tree runtime).
  if (IsRuntimeBuiltin(Name)) {
    return true;
  }

  // Check for predefined Ada types
  if (Name == "Integer" || Name == "Natural" || Name == "Positive" ||
      Name == "Short_Integer" || Name == "Long_Integer" ||
      Name == "Long_Long_Integer" || Name == "Short_Short_Integer") {
    return true;
  }
  if (Name == "Float" || Name == "Long_Float" || Name == "Short_Float" ||
      Name == "Long_Long_Float" || Name == "Short_Short_Float") {
    return true;
  }
  if (Name == "Boolean" || Name == "Character" || Name == "String" ||
      Name == "Wide_Character" || Name == "Wide_String" ||
      Name == "Wide_Wide_Character" || Name == "Wide_Wide_String") {
    return true;
  }
  if (Name == "Null" || Name == "Duration" || Name == "Time") {
    return true;
  }
  if (Name == "Access") {
    return true;
  }

  return false;
}

SubprogramDeclaration *SemanticAnalyzer::ResolveOverload(
    const std::string &Name, const std::vector<TypeNode *> &ArgTypes) {
  // Collect all candidate overloads from all scopes
  std::vector<SubprogramDeclaration *> Candidates;
  
  for (size_t i = ScopeStack.size(); i > 0; --i) {
    auto Found = ScopeStack[i - 1].find(Name);
    if (Found != ScopeStack[i - 1].end()) {
      for (DeclarationNode *Decl : Found->second) {
        if (Decl && Decl->getKind() == NodeKind::SubprogramDeclaration) {
          auto *Subprogram = static_cast<SubprogramDeclaration *>(Decl);
          // A call may omit trailing parameters that have defaults (RM 6.4).
          bool CountOk = ArgTypes.size() <= Subprogram->FormalParameters.size();
          if (CountOk) {
            for (size_t k = ArgTypes.size();
                 k < Subprogram->FormalParameters.size(); ++k) {
              auto &Param = Subprogram->FormalParameters[k];
              if (!Param || !Param->Default) {
                CountOk = false;
                break;
              }
            }
          }
          if (CountOk) {
            Candidates.push_back(Subprogram);
          }
        }
      }
    }
  }
  
  if (Candidates.empty()) {
    return nullptr;
  }
  
  if (Candidates.size() == 1) {
    return Candidates[0];
  }
  
  // Multiple candidates - check type compatibility
  std::vector<SubprogramDeclaration *> Viable;
  for (auto *Candidate : Candidates) {
    bool Compatible = true;
    for (size_t i = 0; i < ArgTypes.size(); ++i) {
      if (Candidate->FormalParameters[i] && Candidate->FormalParameters[i]->Type) {
        if (!CheckTypeCompatibility(Candidate->FormalParameters[i]->Type.get(), ArgTypes[i])) {
          Compatible = false;
          break;
        }
        // Tagged overloading (RM 8.3): the actual must be a descendant of
        // (or the same as) the formal's tagged type, so e.g. a Triangle only
        // conforms to a Shape formal, never to a Circle or Square formal.
        if (!IsTaggedAncestorOrSame(
                GetTypeName(Candidate->FormalParameters[i]->Type.get()),
                GetTypeName(ArgTypes[i]))) {
          Compatible = false;
          break;
        }
      }
    }
    if (Compatible) {
      Viable.push_back(Candidate);
    }
  }
  
  if (Viable.size() == 1) {
    return Viable[0];
  }
  
  // Ambiguous or no viable candidates
  if (Viable.empty()) {
    return nullptr;
  }

  // Multiple viable candidates: prefer the one whose first formal parameter
  // is the most specific tagged type (a descendant beats its ancestors).
  SubprogramDeclaration *Best = Viable[0];
  int BestDepth = TaggedDepthOfFormal(Best);
  for (size_t i = 1; i < Viable.size(); ++i) {
    int Depth = TaggedDepthOfFormal(Viable[i]);
    if (Depth > BestDepth) {
      BestDepth = Depth;
      Best = Viable[i];
    }
  }
  return Best;
}

TypeNode *SemanticAnalyzer::InferType(ExpressionNode *Expr) {
  if (!Expr) {
    return nullptr;
  }

  // If type already inferred, return it
  if (Expr->Type) {
    return Expr->Type.get();
  }

  // Infer based on expression kind
  switch (Expr->getKind()) {
  case NodeKind::IdentifierExpression: {
    auto Id = static_cast<IdentifierExpression *>(Expr);
    DeclarationNode *Decl = ResolveName(Id->Name, 0);
    if (Decl && Decl->getKind() == NodeKind::ObjectDeclaration) {
      auto *ObjDecl = static_cast<ObjectDeclaration *>(Decl);
      return ObjDecl->Type.get();
    }
    // Type of parameterless runtime builtins used as expressions.
    if (IsRuntimeBuiltin(Id->Name)) {
      std::string N = ToLowerName(Id->Name);
      if (N == "clock") {
        auto T = std::make_unique<ScalarType>();
        T->ScalarKind = ScalarType::Float;
        T->Name = "Float";
        Expr->Type = std::move(T);
        return Expr->Type.get();
      }
      if (N == "length" || N == "string_length" || N == "env_exists" ||
          N == "environment_exists" || N == "exists") {
        auto T = std::make_unique<ScalarType>();
        if (N == "exists") {
          T->ScalarKind = ScalarType::Boolean;
          T->Name = "Boolean";
        } else {
          T->ScalarKind = ScalarType::Integer;
          T->Name = "Integer";
        }
        Expr->Type = std::move(T);
        return Expr->Type.get();
      }
    }
    break;
  }
  case NodeKind::TypeConversionExpression: {
    auto Conversion = static_cast<TypeConversionExpression *>(Expr);
    // Type conversion result is the target type
    if (Conversion->TargetType) {
      return Conversion->TargetType.get();
    }
    break;
  }
  case NodeKind::AttributeReference: {
    auto Attr = static_cast<AttributeReference *>(Expr);
    // For P3 the only attribute in use is 'Result; its type is the type of
    // the prefix expression (in practice the enclosing subprogram's result).
    if (Attr->Prefix) {
      TypeNode *T = InferType(Attr->Prefix.get());
      if (T) {
        return T;
      }
    }
    break;
  }
  case NodeKind::LiteralExpression: {
    auto Lit = static_cast<LiteralExpression *>(Expr);
    switch (Lit->LiteralKind) {
    case LiteralExpression::Integer: {
      auto IntType = std::make_unique<ScalarType>();
      IntType->ScalarKind = ScalarType::Integer;
      IntType->Name = "Integer";
      Expr->Type = std::move(IntType);
      return Expr->Type.get();
    }
    case LiteralExpression::Float: {
      auto RealType = std::make_unique<ScalarType>();
      RealType->ScalarKind = ScalarType::Float;
      RealType->Name = "Float";
      Expr->Type = std::move(RealType);
      return Expr->Type.get();
    }
    case LiteralExpression::Boolean: {
      auto BoolType = std::make_unique<ScalarType>();
      BoolType->ScalarKind = ScalarType::Boolean;
      BoolType->Name = "Boolean";
      Expr->Type = std::move(BoolType);
      return Expr->Type.get();
    }
    case LiteralExpression::Character: {
      auto CharType = std::make_unique<ScalarType>();
      CharType->ScalarKind = ScalarType::Character;
      CharType->Name = "Character";
      Expr->Type = std::move(CharType);
      return Expr->Type.get();
    }
    case LiteralExpression::String: {
      // Strings are arrays of characters in Ada
      auto StringType = std::make_unique<ArrayType>();
      Expr->Type = std::move(StringType);
      return Expr->Type.get();
    }
    default:
      break;
    }
    break;
  }
  case NodeKind::NullExpression: {
    // Null can be assigned to any access type
    return nullptr;
  }
  default:
    break;
  }

  return nullptr;
}