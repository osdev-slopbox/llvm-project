//===--- AST.cpp - Ada AST node implementations -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/AST/AST.h"
#include <iostream>

namespace clang {
namespace ada {

// Declarations
void PackageDeclaration::dump() const {
  std::cout << "PackageDeclaration: " << Name;
  if (IsBody) std::cout << " (body)";
  if (IsRenamed) std::cout << " renamed " << RenamedPackage;
  std::cout << "\n";
  for (auto &Decl : Declarations) Decl->dump();
  for (auto &Stmt : BodyStatements) Stmt->dump();
}

void SubprogramDeclaration::dump() const {
  std::cout << "SubprogramDeclaration: " << Name;
  if (IsFunction) std::cout << " (function)";
  if (IsBody) std::cout << " (body)";
  if (IsAbstract) std::cout << " (abstract)";
  std::cout << "\n";
  for (auto &Param : FormalParameters) Param->dump();
  if (ReturnType) ReturnType->dump();
  for (auto &Decl : Declarations) Decl->dump();
  for (auto &Stmt : BodyStatements) Stmt->dump();
  for (auto &Aspect : Aspects) Aspect->dump();
}

void FormalParameter::dump() const {
  std::cout << "FormalParameter: " << Name;
  if (IsInout) std::cout << " (in out)";
  std::cout << "\n";
  if (Type) Type->dump();
  if (Default) Default->dump();
}

void ObjectDeclaration::dump() const {
  std::cout << "ObjectDeclaration: " << Name;
  if (IsConstant) std::cout << " (constant)";
  std::cout << "\n";
  if (Type) Type->dump();
  if (Initializer) Initializer->dump();
  for (auto &Aspect : Aspects) Aspect->dump();
}

void GenericInstantiation::dump() const {
  std::cout << "GenericInstantiation: " << Name << " is new " << GenericName << "\n";
  for (auto &Assoc : Actuals) {
    std::cout << "  " << Assoc.first << " => ";
    if (Assoc.second) {
      Assoc.second->dump();
    } else {
      std::cout << "<null>\n";
    }
  }
}

void TaskDeclaration::dump() const {
  std::cout << "TaskDeclaration: " << Name;
  if (IsBody) std::cout << " (with body)";
  std::cout << "\n";
  for (auto &Entry : Entries) Entry->dump();
  if (Body) Body->dump();
}

void TaskBody::dump() const {
  std::cout << "TaskBody: " << Name << "\n";
  for (auto &Decl : Declarations) Decl->dump();
  for (auto &Stmt : BodyStatements) Stmt->dump();
}

void EntryDeclaration::dump() const {
  std::cout << "EntryDeclaration: " << Name << "\n";
  for (auto &Param : FormalParameters) Param->dump();
}

void AcceptStatement::dump() const {
  std::cout << "AcceptStatement: " << EntryName << "\n";
  for (auto &Param : FormalParameters) Param->dump();
  for (auto &Stmt : Body) Stmt->dump();
}

void DelayStatement::dump() const {
  std::cout << "DelayStatement\n";
  if (Duration) Duration->dump();
}

void RequeueStatement::dump() const {
  std::cout << "RequeueStatement: " << EntryName;
  if (IsOthers) std::cout << " (others)";
  std::cout << "\n";
}

void PragmaStatement::dump() const {
  std::cout << "PragmaStatement: " << PragmaName << "\n";
}

void TypeDeclaration::dump() const {
  std::cout << "TypeDeclaration: " << Name;
  if (IsPrivate) std::cout << " (private)";
  if (IsTagged) std::cout << " (tagged)";
  if (IsAbstract) std::cout << " (abstract)";
  if (!Parent.empty()) std::cout << " new " << Parent;
  std::cout << "\n";
  if (Type) Type->dump();
  for (auto &Aspect : Aspects) Aspect->dump();
}

// Types
void ScalarType::dump() const {
  std::cout << "ScalarType: " << Name;
  switch (ScalarKind) {
  case Integer: std::cout << " (integer)"; break;
  case Float: std::cout << " (float)"; break;
  case Character: std::cout << " (character)"; break;
  case Enumeration: std::cout << " (enumeration)"; break;
  case Boolean: std::cout << " (boolean)"; break;
  }
  std::cout << "\n";
}

void ArrayType::dump() const {
  std::cout << "ArrayType\n";
  if (ElementType) ElementType->dump();
}

void RecordType::dump() const {
  std::cout << "RecordType\n";
  for (auto &Field : Fields) Field->dump();
}

void AccessType::dump() const {
  std::cout << "AccessType";
  if (TargetType) TargetType->dump();
}

void StringType::dump() const {
  std::cout << "StringType\n";
  if (ElementType) ElementType->dump();
}

void DerivedType::dump() const {
  std::cout << "DerivedType from " << ParentType << "\n";
  for (auto &Ext : Extensions) Ext->dump();
}

void DiscriminatedType::dump() const {
  std::cout << "DiscriminatedType\n";
  for (auto &Disc : Discriminants) Disc->dump();
}

// Expressions
void LiteralExpression::dump() const {
  std::cout << "LiteralExpression: ";
  switch (LiteralKind) {
  case Integer: std::cout << "integer " << Value; break;
  case Float: std::cout << "float " << Value; break;
  case String: std::cout << "string \"" << Value << "\""; break;
  case Character: std::cout << "character '" << CharacterValue << "'"; break;
  case Boolean: std::cout << "boolean " << (BooleanValue ? "true" : "false"); break;
  case Null: std::cout << "null"; break;
  }
  std::cout << "\n";
}

void IdentifierExpression::dump() const {
  std::cout << "IdentifierExpression: ";
  for (const auto &Qual : Qualifiers) std::cout << Qual << ".";
  std::cout << Name << "\n";
}

void BinaryExpression::dump() const {
  std::cout << "BinaryExpression: ";
  switch (Op) {
  case Add: std::cout << "+"; break;
  case Subtract: std::cout << "-"; break;
  case Multiply: std::cout << "*"; break;
  case Divide: std::cout << "/"; break;
  case Mod: std::cout << " mod "; break;
  case Rem: std::cout << " rem "; break;
  case Power: std::cout << "**"; break;
  case And: std::cout << " and "; break;
  case Or: std::cout << " or "; break;
  case Xor: std::cout << " xor "; break;
  case Nand: std::cout << " nand "; break;
  case Nor: std::cout << " nor "; break;
  case Equal: std::cout << "="; break;
  case NotEqual: std::cout << "/="; break;
  case LessThan: std::cout << "<"; break;
  case GreaterThan: std::cout << ">"; break;
  case LessEqual: std::cout << "<="; break;
  case GreaterEqual: std::cout << ">="; break;
  case Concat: std::cout << " & "; break;
  case In: std::cout << " in "; break;
  case Range: std::cout << " .. "; break;
  }
  std::cout << "\n";
  if (Left) Left->dump();
  if (Right) Right->dump();
}

void UnaryExpression::dump() const {
  std::cout << "UnaryExpression: ";
  switch (Op) {
  case Negate: std::cout << "-"; break;
  case Positive: std::cout << "+"; break;
  case Not: std::cout << "not "; break;
  }
  std::cout << "\n";
  if (Operand) Operand->dump();
}

void CallExpression::dump() const {
  std::cout << "CallExpression\n";
  if (Function) Function->dump();
  for (auto &Arg : Arguments) {
    std::cout << "  arg: ";
    Arg->dump();
  }
}

void TypeConversionExpression::dump() const {
  std::cout << "TypeConversionExpression\n";
  if (TargetType) TargetType->dump();
  if (Operand) Operand->dump();
}

void AggregateExpression::dump() const {
  std::cout << "AggregateExpression\n";
  for (auto &Elem : Elements) Elem->dump();
}

void ArrayElementAccess::dump() const {
  std::cout << "ArrayElementAccess\n";
  if (Array) Array->dump();
  if (Index) Index->dump();
}

void RecordFieldAccess::dump() const {
  std::cout << "RecordFieldAccess: " << FieldName << "\n";
  if (Record) Record->dump();
}

void AttributeReference::dump() const {
  std::cout << "AttributeReference: " << AttributeName << "\n";
  if (Prefix) Prefix->dump();
}

void CaseExpression::dump() const {
  std::cout << "CaseExpression\n";
  if (Discriminant) Discriminant->dump();
  for (auto &Choice : Choices) Choice->dump();
  for (auto &Result : Results) Result->dump();
}

void ConditionalExpression::dump() const {
  std::cout << "ConditionalExpression\n";
  if (Condition) Condition->dump();
  if (ThenExpr) ThenExpr->dump();
  if (ElseExpr) ElseExpr->dump();
}

void NullExpression::dump() const {
  std::cout << "NullExpression\n";
}

// Statements
void AssignmentStatement::dump() const {
  std::cout << "AssignmentStatement\n";
  if (Target) Target->dump();
  if (Value) Value->dump();
}

void IfStatement::dump() const {
  std::cout << "IfStatement\n";
  if (Condition) Condition->dump();
  for (auto &Stmt : ThenStatements) Stmt->dump();
  for (auto &Elsif : Elsifs) {
    Elsif.first->dump();
    for (auto &Stmt : Elsif.second) Stmt->dump();
  }
  for (auto &Stmt : ElseStatements) Stmt->dump();
}

void LoopStatement::dump() const {
  std::cout << "LoopStatement\n";
  for (auto &Stmt : Body) Stmt->dump();
}

void ForLoopStatement::dump() const {
  std::cout << "ForLoopStatement: " << Variable;
  if (IsReverse) std::cout << " reverse";
  std::cout << "\n";
  if (Low) Low->dump();
  if (High) High->dump();
  for (auto &Stmt : Body) Stmt->dump();
}

void WhileLoopStatement::dump() const {
  std::cout << "WhileLoopStatement\n";
  if (Condition) Condition->dump();
  for (auto &Stmt : Body) Stmt->dump();
}

void CaseStatement::dump() const {
  std::cout << "CaseStatement\n";
  if (Discriminant) Discriminant->dump();
  for (auto &Alt : Alternatives) {
    for (auto &Choice : Alt.first) Choice->dump();
    for (auto &Stmt : Alt.second) Stmt->dump();
  }
}

void BlockStatement::dump() const {
  std::cout << "BlockStatement: " << Label << "\n";
  for (auto &Decl : Declarations) Decl->dump();
  for (auto &Stmt : Statements) Stmt->dump();
}

void ProcedureCallStatement::dump() const {
  std::cout << "ProcedureCallStatement\n";
  if (Procedure) Procedure->dump();
  for (auto &Arg : Arguments) {
    std::cout << "  arg: ";
    Arg->dump();
  }
}

void ReturnStatement::dump() const {
  std::cout << "ReturnStatement\n";
  if (Value) Value->dump();
}

void ExitStatement::dump() const {
  std::cout << "ExitStatement: " << LoopLabel << "\n";
  if (Condition) Condition->dump();
}

void NullStatement::dump() const {
  std::cout << "NullStatement\n";
}

void RaiseStatement::dump() const {
  std::cout << "RaiseStatement\n";
  if (Exception) Exception->dump();
}

void TryStatement::dump() const {
  std::cout << "TryStatement\n";
  for (const auto &S : Body) S->dump();
  for (const auto &H : Handlers) {
    std::cout << "Handler: " << (H.first.empty() ? std::string("others")
                                                  : H.first)
              << "\n";
    for (const auto &S : H.second) S->dump();
  }
}

// Aspects/Pragmas
void AspectSpecification::dump() const {
  std::cout << "AspectSpecification: " << AspectName << "\n";
  if (AspectValue) AspectValue->dump();
}

void PragmaDeclaration::dump() const {
  std::cout << "PragmaDeclaration: " << PragmaName << "\n";
  for (auto &Arg : Arguments) Arg->dump();
}

} // namespace ada
} // namespace clang