//===--- Types.cpp - Ada type AST implementation --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/AST/Types.h"
#include "clang/Ada/AST/Expressions.h"
#include "clang/Ada/AST/Declarations.h"
#include "clang/Ada/AST/Aspects.h"
#include <iostream>

namespace clang {
namespace ada {

void ScalarType::dump() const {
  std::cout << "ScalarType: " << Name;
  switch (Kind) {
  case Integer: std::cout << " (integer)"; break;
  case Float: std::cout << " (float)"; break;
  case Character: std::cout << " (character)"; break;
  case Enumeration: std::cout << " (enumeration)"; break;
  case Boolean: std::cout << " (boolean)"; break;
  }
  if (Low) {
    std::cout << " range ";
    Low->dump();
    if (High) {
      std::cout << " .. ";
      High->dump();
    }
  }
  std::cout << "\n";
}

void ArrayType::dump() const {
  std::cout << "ArrayType\n";
  if (ElementType) {
    std::cout << "  element type:\n";
    ElementType->dump();
  }
  for (auto &Idx : IndexExpressions) {
    std::cout << "  index: ";
    Idx->dump();
  }
}

void RecordType::dump() const {
  std::cout << "RecordType\n";
  for (auto &Field : Fields) {
    Field->dump();
  }
}

void AccessType::dump() const {
  std::cout << "AccessType: ";
  if (TargetType) {
    TargetType->dump();
  }
}

void StringType::dump() const {
  std::cout << "StringType\n";
  if (ElementType) {
    ElementType->dump();
  }
}

void DerivedType::dump() const {
  std::cout << "DerivedType from " << ParentType << "\n";
}

void DiscriminatedType::dump() const {
  std::cout << "DiscriminatedType\n";
  for (auto &Disc : Discriminants) {
    Disc->dump();
  }
}

} // namespace ada
} // namespace clang
