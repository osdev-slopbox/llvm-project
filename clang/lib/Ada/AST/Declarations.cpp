//===--- Declarations.cpp - Ada declaration AST implementation --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/AST/Declarations.h"
#include "clang/Ada/AST/Types.h"
#include "clang/Ada/AST/Expressions.h"
#include "clang/Ada/AST/Statements.h"
#include "clang/Ada/AST/Aspects.h"
#include <iostream>

namespace clang {
namespace ada {

void PackageDeclaration::dump() const {
  std::cout << "PackageDeclaration: " << Name;
  if (IsBody) std::cout << " (body)";
  if (IsRenamed) std::cout << " renamed " << RenamedPackage;
  std::cout << "\n";
  for (auto &Decl : Declarations) {
    Decl->dump();
  }
  for (auto &Stmt : BodyStatements) {
    Stmt->dump();
  }
}

void SubprogramDeclaration::dump() const {
  std::cout << "SubprogramDeclaration: " << Name;
  if (IsFunction) std::cout << " (function)";
  if (IsBody) std::cout << " (body)";
  if (IsAbstract) std::cout << " (abstract)";
  if (IsAsync) std::cout << " (async)";
  if (IsProtected) std::cout << " (protected)";
  if (HasConcurrent) std::cout << " (concurrent)";
  std::cout << "\n";
  for (auto &Param : FormalParameters) {
    Param->dump();
  }
  if (ReturnType) {
    ReturnType->dump();
  }
  for (auto &Decl : Declarations) {
    Decl->dump();
  }
  for (auto &Stmt : BodyStatements) {
    Stmt->dump();
  }
}

void ObjectDeclaration::dump() const {
  std::cout << "ObjectDeclaration: " << Name;
  if (IsConstant) std::cout << " (constant)";
  std::cout << "\n";
  if (Type) {
    Type->dump();
  }
  if (Initializer) {
    Initializer->dump();
  }
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

void TypeDeclaration::dump() const {
  std::cout << "TypeDeclaration: " << Name;
  if (IsPrivate) std::cout << " (private)";
  std::cout << "\n";
  if (Type) {
    Type->dump();
  }
}

void FormalParameter::dump() const {
  std::cout << "FormalParameter: " << Name;
  if (IsInout) std::cout << " (in out)";
  std::cout << "\n";
  if (Type) {
    Type->dump();
  }
  if (Default) {
    Default->dump();
  }
}

} // namespace ada
} // namespace clang
