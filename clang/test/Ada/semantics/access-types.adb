// RUN: %clang_ada -fsyntax-only -a2012 %s
-- Access types (RM 3.10)
procedure Access_Types is
    type Int_Access is access Integer;
    P : Int_Access;
begin
    P := null;
    null;
end Access_Types;
