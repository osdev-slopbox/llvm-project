-- Test: switch optimization opportunity
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure switch_optimization is
    x : Integer := 2;
    result : Integer := 0;
begin
    case x is
        when 1 => result := 10;
        when 2 => result := 20;
        when 3 => result := 30;
        when others => result := 0;
    end case;
    null;
end switch_optimization;
