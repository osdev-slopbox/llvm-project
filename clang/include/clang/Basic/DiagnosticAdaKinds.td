//==--- DiagnosticAdaKinds.td - Ada Diagnostics ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the Ada-specific diagnostics for the clang-ada frontend.
//
//===----------------------------------------------------------------------===//

let Component = "Ada" in {

def err_ada_not_implemented : Error<
  "ac1: the Ada 2012 frontend is not implemented yet">;

} // end Component "Ada"
