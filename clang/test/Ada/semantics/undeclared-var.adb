// RUN: not %clang_ada -fsyntax-only -a2012 %s 2>&1 | FileCheck %s
-- Undeclared variable
procedure Undeclared_Var is
    X : Integer := 10;
begin
    X := Y + 5;
end Undeclared_Var;

// CHECK: error
