//===--- ac1_main.cpp - Ada 2012 frontend entry point ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The entry point for the "ac1" (Ada compiler) frontend tool, invoked as
// "clang -ac1" or "clang-ada -ac1". Parses command-line arguments and
// dispatches to the appropriate frontend action (compile, syntax-only,
// dependency-scan, ast-dump).
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/Frontend/AdaFrontendAction.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticAda.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

using namespace clang;

int ac1_main(llvm::ArrayRef<const char *> Argv, const char *Argv0, void *MainAddr) {
  (void)MainAddr;

  int Argc = Argv.size();
  const char * const *ArgArray = Argv.data();

  clang::ada::AdaFrontendOptions Opts;
  if (!clang::ada::ParseAc1Args(Argc, ArgArray, Opts)) {
    std::cerr << "ac1: error: failed to parse arguments\n";
    return 1;
  }

  return clang::ada::RunFrontendAction(Opts);
}