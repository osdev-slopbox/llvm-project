// RUN: %clang_ada -fsyntax-only -a2012 %s
-- Type conversion test (RM 3.10)
procedure Type_Conversion is
    I : Integer := 42;
    F : Float := 3.14;
    B : Boolean := True;
    C : Character := 'A';
begin
    -- Explicit conversions
    F := Float(I);
    I := Integer(F);
    B := Boolean(I);
    C := Character(I);
end Type_Conversion;
