// RUN: not %clang_ada -fsyntax-only -a2012 %s 2>&1 | FileCheck %s
-- Type error: assigning Integer to Boolean
procedure Type_Error is
    X : Integer := 10;
    Y : Boolean;
begin
    Y := X;
end Type_Error;

// CHECK: error
