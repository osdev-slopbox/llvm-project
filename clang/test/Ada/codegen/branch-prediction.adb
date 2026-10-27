-- Test: branch prediction hints
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure branch_prediction is
    x : Integer := 5;
begin
    if x > 0 then
        null;
    end if;
end branch_prediction;
