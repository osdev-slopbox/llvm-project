// RUN: %clang_ada -fsyntax-only -a2012 %s
-- If statement with proper boolean conditions
procedure If_Statement is
    X : Integer := 10;
    Y : Integer := 20;
begin
    if X > 5 then
        Y := X + 10;
    else
        Y := X - 10;
    end if;
end If_Statement;
