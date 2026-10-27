//===--- Statements.cpp - Ada statement AST implementation ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/AST/Statements.h"
#include "clang/Ada/AST/Expressions.h"
#include "clang/Ada/AST/Declarations.h"
#include <iostream>

namespace clang {
namespace ada {

void AssignmentStatement::dump() const {
  std::cout << "AssignmentStatement\n";
  if (Target) {
    Target->dump();
  }
  if (Value) {
    Value->dump();
  }
}

void IfStatement::dump() const {
  std::cout << "IfStatement\n";
  if (Condition) {
    Condition->dump();
  }
  for (auto &Stmt : ThenStatements) {
    Stmt->dump();
  }
  for (auto &Elsif : Elsifs) {
    Elsif.first->dump();
    for (auto &Stmt : Elsif.second) {
      Stmt->dump();
    }
  }
  for (auto &Stmt : ElseStatements) {
    Stmt->dump();
  }
}

void LoopStatement::dump() const {
  std::cout << "LoopStatement\n";
  for (auto &Stmt : Body) {
    Stmt->dump();
  }
}

void ForLoopStatement::dump() const {
  std::cout << "ForLoopStatement: " << Variable;
  if (IsReverse) std::cout << " reverse";
  std::cout << "\n";
  if (Low) Low->dump();
  if (High) High->dump();
  for (auto &Stmt : Body) {
    Stmt->dump();
  }
}

void WhileLoopStatement::dump() const {
  std::cout << "WhileLoopStatement\n";
  if (Condition) {
    Condition->dump();
  }
  for (auto &Stmt : Body) {
    Stmt->dump();
  }
}

void CaseStatement::dump() const {
  std::cout << "CaseStatement\n";
  if (Discriminant) {
    Discriminant->dump();
  }
  for (auto &Alt : Alternatives) {
    for (auto &Choice : Alt.first) {
      Choice->dump();
    }
    for (auto &Stmt : Alt.second) {
      Stmt->dump();
    }
  }
}

void BlockStatement::dump() const {
  std::cout << "BlockStatement: " << Label << "\n";
  for (auto &Decl : Declarations) {
    Decl->dump();
  }
  for (auto &Stmt : Statements) {
    Stmt->dump();
  }
}

void ProcedureCallStatement::dump() const {
  std::cout << "ProcedureCallStatement\n";
  if (Procedure) {
    Procedure->dump();
  }
  for (auto &Arg : Arguments) {
    std::cout << "  arg: ";
    Arg->dump();
  }
}

void ReturnStatement::dump() const {
  std::cout << "ReturnStatement\n";
  if (Value) {
    Value->dump();
  }
}

void ExitStatement::dump() const {
  std::cout << "ExitStatement: " << LoopLabel << "\n";
  if (Condition) {
    Condition->dump();
  }
}

void NullStatement::dump() const {
  std::cout << "NullStatement\n";
}

void RaiseStatement::dump() const {
  std::cout << "RaiseStatement\n";
  if (Exception) {
    Exception->dump();
  }
}

} // namespace ada
} // namespace clang
