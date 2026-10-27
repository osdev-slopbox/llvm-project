-- Test: early return
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure early_return is
    function find(x : Integer) return Integer is
    begin
        if x = 42 then
            return 1;
        end if;
        return 0;
    end find;
begin
    null;
end early_return;
