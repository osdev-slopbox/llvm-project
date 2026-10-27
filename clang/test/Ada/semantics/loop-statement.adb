// RUN: %clang_ada -fsyntax-only -a2012 %s
-- Loop with proper semantic checks
procedure Loop_Statement is
    Sum : Integer := 0;
begin
    for I in 1 .. 10 loop
        Sum := Sum + I;
    end loop;
end Loop_Statement;
