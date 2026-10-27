-- Test: case expression
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure case_expr is
    x : Integer := 1;
    result : Integer;
begin
    result := case x is
        when 1 => 10
        when 2 => 20
        when others => 0
    end case;
end case_expr;
