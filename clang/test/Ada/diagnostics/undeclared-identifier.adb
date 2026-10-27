-- Test: undeclared identifier error (P8)
-- RUN: not %clang_ada -a2012 -fsyntax-only %s 2>&1 | FileCheck %s
-- CHECK: error: undeclared identifier: Undefined_Var

procedure test_undeclared is
begin
    Undefined_Var := 42;
end test_undeclared;