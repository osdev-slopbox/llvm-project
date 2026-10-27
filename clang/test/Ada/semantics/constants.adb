// RUN: %clang_ada -fsyntax-only -a2012 %s
-- Constant declarations (RM 3.5)
procedure Constants is
    Max_Size : constant Integer := 100;
    Pi : constant Float := 3.14159;
    X : Integer;
begin
    X := Max_Size;
    null;
end Constants;
