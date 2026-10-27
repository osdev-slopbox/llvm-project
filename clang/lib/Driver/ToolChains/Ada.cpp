//===--- Ada.cpp - Ada 2012 Tool and ToolChain Implementations ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the Ada 2012 tool for the Clang driver. The tool
// builds an `-ac1` invocation of the clang-ada frontend (dispatched by
// clang_main before driver option parsing), mirroring how the Clang tool
// invokes `-cc1`.
//
//===----------------------------------------------------------------------===//

#include "Ada.h"
#include "clang/Driver/Action.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/InputInfo.h"
#include "clang/Driver/Job.h"
#include "clang/Driver/ToolChain.h"
#include "clang/Driver/Types.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Path.h"
#include "llvm/ADT/SmallString.h"

#include <cassert>
#include <iostream>

using namespace clang;
using namespace clang::driver;
using namespace clang::driver::tools;
using namespace llvm::opt;

Ada::Ada(const ToolChain &TC) : Tool("ada", "Ada 2012 frontend", TC) {}

void Ada::ConstructJob(Compilation &C, const JobAction &JA,
                       const InputInfo &Output, const InputInfoList &Inputs,
                       const ArgList &Args, const char *LinkingOutput) const {
  const ToolChain &TC = getToolChain();
  const llvm::Triple &Triple = TC.getEffectiveTriple();

  const Driver &D = TC.getDriver();
  ArgStringList CmdArgs;

  // For link jobs, use gcc as the linker.
  if (isa<LinkJobAction>(JA)) {
    CmdArgs.push_back("-o");
    CmdArgs.push_back(Output.getFilename());

    // Add all object files
    for (const InputInfo &II : Inputs) {
      if (II.isFilename())
        CmdArgs.push_back(II.getFilename());
    }

    // Add the Ada runtime library
    CmdArgs.push_back(Args.MakeArgString(std::string("-L") + D.ResourceDir));
    CmdArgs.push_back("-lclangAdaRuntime");

    // Forward -L and -l flags from the user
    for (const Arg *A : Args.filtered(options::OPT_L)) {
      CmdArgs.push_back("-L");
      CmdArgs.push_back(A->getValue());
    }
    for (const Arg *A : Args.filtered(options::OPT_l)) {
      CmdArgs.push_back("-l");
      CmdArgs.push_back(A->getValue());
    }

    CmdArgs.push_back("-lc");

    // Use gcc as the linker (it knows where the C runtime files are)
    const char *Gcc = "gcc";
    C.addCommand(std::make_unique<Command>(
        JA, *this, ResponseFileSupport::AtFileUTF8(), Gcc, CmdArgs, Inputs,
        Output, D.getPrependArg()));
    return;
  }

  // For non-link jobs, invoke the ac1 frontend.
  CmdArgs.push_back("-ac1");

  // Add the "effective" target triple.
  CmdArgs.push_back("-triple");
  CmdArgs.push_back(Args.MakeArgString(Triple.getTriple()));

  // Forward contract-control flags to ac1 (every Ada job runs ac1; the
  // backend .bc job falls through to the cc1 tool, which never sees these).
  if (Args.hasArg(options::OPT_fno_ada_contracts))
    CmdArgs.push_back("-fno-ada-contracts");

  // Forward Ada version flag to ac1
  if (Args.hasArg(options::OPT_a2012))
    CmdArgs.push_back("-a2012");

  // Forward optimization level to ac1
  if (Arg *A = Args.getLastArg(options::OPT_O_Group)) {
    A->render(Args, CmdArgs);
  }

  // Forward warning flags to ac1. All -W* options live in W_Group; forward
  // the whole group so -Wall/-Wextra/-Wpedantic/-Werror/-Wno-ada-* reach ac1
  // (and are marked "used" so the driver does not warn about them).
  Args.AddAllArgs(CmdArgs, options::OPT_W_Group);

  // Pass the original source file path to ac1 for module file emission
  // Pass for TY_Ada, TY_PP_Ada, and TY_LLVM_BC
  if (JA.getType() == types::TY_Ada || JA.getType() == types::TY_PP_Ada ||
      JA.getType() == types::TY_LLVM_BC) {
    const InputInfo &Input = Inputs[0];
    if (Input.isFilename()) {
      CmdArgs.push_back(Args.MakeArgString(std::string("-original-source=") +
                                           Input.getBaseInput()));
    }
  }

  // Forward -M/-MD/-MMD and related dependency-scan flags to ac1 only for
  // the preprocess job, not the compile job. The compile job should perform
  // a syntax check, not a dependency scan.
  if (JA.getKind() == Action::PreprocessJobClass) {
    if (Args.hasArg(options::OPT_M))
      CmdArgs.push_back("-M");
    if (Args.hasArg(options::OPT_MD))
      CmdArgs.push_back("-MD");
    if (Args.hasArg(options::OPT_MMD))
      CmdArgs.push_back("-MMD");
    if (const Arg *A = Args.getLastArg(options::OPT_MF))
      CmdArgs.push_back(Args.MakeArgString(std::string("-MF=") + A->getValue()));
    if (const Arg *A = Args.getLastArg(options::OPT_MT))
      CmdArgs.push_back(Args.MakeArgString(std::string("-MT=") + A->getValue()));
    if (const Arg *A = Args.getLastArg(options::OPT_MQ))
      CmdArgs.push_back(Args.MakeArgString(std::string("-MQ=") + A->getValue()));

    // Add a default target for dependency scanning if not specified.
    bool HasTarget = Args.hasArg(options::OPT_MT) || Args.hasArg(options::OPT_MQ);
    if (!HasTarget && (Args.hasArg(options::OPT_M) || Args.hasArg(options::OPT_MD) ||
                       Args.hasArg(options::OPT_MMD))) {
      const InputInfo &Input = Inputs[0];
      if (Input.isFilename()) {
        llvm::SmallString<128> P(Input.getBaseInput());
        llvm::sys::path::replace_extension(P, "o");
        CmdArgs.push_back(Args.MakeArgString(std::string("-MT=") +
                                             llvm::sys::path::filename(P)));
      }
    }
  }

  // Debug: print the command line
  std::cerr << "Ada::ConstructJob - input: ";
  for (const InputInfo &II : Inputs) {
    if (II.isFilename())
      std::cerr << II.getFilename();
  }
  std::cerr << "\n";

  // Map the job action to an ac1 action.
  if (isa<PreprocessJobAction>(JA)) {
    CmdArgs.push_back("-emit-ada-preprocessed");
  } else if (JA.getType() == types::TY_Object ||
             JA.getType() == types::TY_LLVM_BC ||
             JA.getType() == types::TY_LTO_IR) {
    // For compile/backend jobs, always emit object directly
    CmdArgs.push_back("-emit-obj");
  } else {
    std::cerr << "Ada::ConstructJob: unknown job action type "
              << static_cast<int>(JA.getType()) << "\n";
    CmdArgs.push_back("-fsyntax-only");
  }

  assert((Output.isFilename() || Output.isNothing()) && "Invalid output.");
  if (Output.isFilename()) {
    CmdArgs.push_back("-o");
    CmdArgs.push_back(Output.getFilename());
  }

  const InputInfo &Input = Inputs[0];

  // The input could be Ty_Nothing when "querying" options such as -mcpu=help
  // are used.
  ArrayRef<InputInfo> FrontendInputs = Input;
  if (Input.isNothing())
    FrontendInputs = {};

  for (const InputInfo &II : FrontendInputs) {
    if (II.isFilename())
      CmdArgs.push_back(II.getFilename());
    else
      II.getInputArg().renderAsInput(Args, CmdArgs);
  }

  const char *Exec = D.getDriverProgramPath();
  C.addCommand(std::make_unique<Command>(
      JA, *this, ResponseFileSupport::AtFileUTF8(), Exec, CmdArgs, Inputs,
      Output, D.getPrependArg()));
}
