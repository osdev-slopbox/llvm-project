-- Test: loop with label
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure labeled_loop is
    x : Integer := 0;
begin
    outer_loop: loop
        exit outer_loop when x >= 10;
        x := x + 1;
    end loop outer_loop;
end labeled_loop;
