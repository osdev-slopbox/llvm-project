-- Test: warning levels -Wall / -Wextra and -Wno-ada-* (P8)
-- RUN: %clang_ada -a2012 -Wextra -fsyntax-only %s 2>&1 | FileCheck %s
-- RUN: %clang_ada -a2012 -Wextra -Wno-ada-unused-parameter -fsyntax-only %s 2>&1 | FileCheck --check-prefix=NOEXTRA %s

procedure Warning_Levels is
   procedure Foo(A : Integer; B : Integer) is
      Unused : Integer := 0;
   begin
      Unused := A + Unused;
   end Foo;
begin
   Foo(1, 2);
end Warning_Levels;

-- -Wextra enables unused-parameter for B (A is used).
CHECK: warning: unused parameter 'B' [-Wunused-parameter]

-- With -Wno-ada-unused-parameter the parameter warning is gone.
NOEXTRA-NOT: unused parameter
