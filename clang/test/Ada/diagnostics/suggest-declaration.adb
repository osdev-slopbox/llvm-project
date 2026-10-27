-- Test: "did you mean" note for undeclared identifiers (P8)
-- RUN: not %clang_ada -a2012 -fsyntax-only %s 2>&1 | FileCheck %s
-- CHECK: error: undeclared identifier: Coutner
-- CHECK: note: did you mean 'Counter'?

procedure Suggest_Declaration is
   Counter : Integer := 0;
begin
   Counter := 1;
   Coutner := 2;
end Suggest_Declaration;
