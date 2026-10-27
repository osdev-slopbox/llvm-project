-- Test: nested blocks
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure nested_blocks is
begin
    declare
        x : Integer := 1;
    begin
        declare
            y : Integer := 2;
        begin
            null;
        end;
    end;
end nested_blocks;
