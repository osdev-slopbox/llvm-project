//===--- Ada.h - Ada 2012 Tool and ToolChain Implementations ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the Ada 2012 tool for the Clang driver. The tool builds
// an `-ac1` invocation of the clang-ada frontend.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ADA_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ADA_H

#include "clang/Driver/Tool.h"
#include "clang/Driver/Action.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/ToolChain.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/Compiler.h"

namespace clang {
namespace driver {

namespace tools {

/// Ada 2012 compiler tool.
class LLVM_LIBRARY_VISIBILITY Ada : public Tool {
public:
  Ada(const ToolChain &TC);
  bool hasIntegratedCPP() const override { return false; }
  void ConstructJob(Compilation &C, const JobAction &JA,
                    const InputInfo &Output, const InputInfoList &Inputs,
                    const llvm::opt::ArgList &TCArgs,
                    const char *LinkingOutput) const override;
};

} // namespace tools

} // namespace driver
} // namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_ADA_H
