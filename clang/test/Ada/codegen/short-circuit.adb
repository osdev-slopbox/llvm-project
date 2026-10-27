-- Test: short-circuit evaluation
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure short_circuit is
    a, b : Integer := 5;
begin
    if a > 0 and b > 0 then
        null;
    end if;
    if a > 10 or b > 10 then
        null;
    end if;
end short_circuit;
