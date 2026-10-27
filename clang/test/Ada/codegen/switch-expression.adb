-- Test: switch-like expression using if
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure switch_expr is
    x : Integer := 2;
    result : String(1..10);
begin
    if x = 1 then
        result := "one";
    else
        if x = 2 then
            result := "two";
        else
            result := "other";
        end if;
    end if;
    null;
end switch_expr;
