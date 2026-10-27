-- Test: multiple return statements
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure multi_return is
    function classify(x : Integer) return Integer is
    begin
        if x < 0 then
            return -1;
        end if;
        if x = 0 then
            return 0;
        end if;
        return 1;
    end classify;
begin
    null;
end multi_return;
