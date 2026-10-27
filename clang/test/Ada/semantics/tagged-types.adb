// RUN: %clang_ada -fsyntax-only -a2012 %s
-- Tagged types and dispatch (RM 3.9)
procedure Tagged_Types is
    type Shape is tagged null record;
    
    procedure Draw(S : access Shape);
    
    type Circle is new Shape with null record;
    
    procedure Draw(S : access Circle);
    
    C : Circle;
begin
    Draw(C'Access);
end Tagged_Types;
