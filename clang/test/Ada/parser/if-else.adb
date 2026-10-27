-- Test: if-elsif-else
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure if_else is
    x : Integer := 5;
begin
    if x < 0 then
        null;
    elsif x = 0 then
        null;
    else
        null;
    end if;
end if_else;
