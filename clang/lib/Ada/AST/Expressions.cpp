//===--- Expressions.cpp - Ada expression AST implementation ---------===//
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

void AllocationExpression::dump() const {
  std::cout << "AllocationExpression\n";
  if (Type) {
    Type->dump();
  }
  if (Initializer) {
    std::cout << "  initializer:\n";
    Initializer->dump();
  }
}

} // namespace ada
} // namespace clang