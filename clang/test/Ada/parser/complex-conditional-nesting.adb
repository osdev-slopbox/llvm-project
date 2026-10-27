-- Test: complex conditional nesting
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_conditional_nesting is
    x, y : Integer := 5;
begin
    if x > 0 then
        if y > 0 then
            null;
        else
            null;
        end if;
    else
        null;
    end if;
end complex_conditional_nesting;
