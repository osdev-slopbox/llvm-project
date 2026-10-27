-- Test: choice list in case
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure choice_list is
    x : Integer := 5;
begin
    case x is
        when 1 | 2 | 3 =>
            null;
        when 4 | 5 | 6 =>
            null;
        when others =>
            null;
    end case;
end choice_list;
