// RUN: not %clang_ada -fsyntax-only -a2012 %s 2>&1 | FileCheck %s
-- Should fail: undeclared variable
procedure Reject_Undeclared is
    X : Integer := 10;
begin
    X := Y + 5;
end Reject_Undeclared;

// CHECK: error
