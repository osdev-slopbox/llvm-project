-- Test: unknown pragma warning (P8)
-- RUN: %clang_ada -a2012 -fsyntax-only %s 2>&1 | FileCheck %s
-- CHECK: warning: unknown pragma: UnknownPragma
-- CHECK: warning: unknown pragma: AnotherUnknown

procedure test_unknown_pragma is
begin
    pragma UnknownPragma;
    pragma AnotherUnknown;
    null;
end test_unknown_pragma;
