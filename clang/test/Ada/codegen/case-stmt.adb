-- Test: case statement
-- RUN: %clang_ada -a2012 -c %s -o /dev/null

procedure case_stmt is
    x : Integer := 2;
    result : String(1..10);
begin
    result := "";
    case x is
        when 1 => result(1) := 'o';
        when 2 => result(1) := 't';
        when 3 => result(1) := 't';
        when others => result(1) := 'o';
    end case;
    Put(result(1));
    New_Line;
end case_stmt;
