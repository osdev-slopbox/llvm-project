//===--- Aspects.cpp - Ada aspect and pragma AST implementation ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/AST/Aspects.h"
#include "clang/Ada/AST/Expressions.h"
#include <iostream>

namespace clang {
namespace ada {

void AspectSpecification::dump() const {
  std::cout << "AspectSpecification: " << AspectName << "\n";
  if (AspectValue) {
    AspectValue->dump();
  }
}

void PreconditionAspect::dump() const {
  std::cout << "PreconditionAspect\n";
  if (AspectValue) {
    AspectValue->dump();
  }
}

void PostconditionAspect::dump() const {
  std::cout << "PostconditionAspect\n";
  if (AspectValue) {
    AspectValue->dump();
  }
}

void InvariantAspect::dump() const {
  std::cout << "InvariantAspect\n";
  if (AspectValue) {
    AspectValue->dump();
  }
}

void PragmaDeclaration::dump() const {
  std::cout << "PragmaDeclaration: " << PragmaName << "\n";
  for (auto &Arg : Arguments) {
    Arg->dump();
  }
}

} // namespace ada
} // namespace clang
