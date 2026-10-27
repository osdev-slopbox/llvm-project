-- Test: complex nested if
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_nested_if is
    x, y : Integer := 5;
begin
    if x > 0 then
        if y > 0 then
            null;
        end if;
    end if;
end complex_nested_if;
