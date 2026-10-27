-- Test: -Werror promotes warnings to errors (P8)
-- RUN: not %clang_ada -a2012 -Werror -fsyntax-only %s 2>&1 | FileCheck %s
-- CHECK: warning: unknown pragma: UnknownPragma
-- CHECK: ac1: error: semantic analysis failed

procedure test_werror is
begin
    pragma UnknownPragma;
    null;
end test_werror;
