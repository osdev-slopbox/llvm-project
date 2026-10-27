-- Test: exception handling
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure exception_handling is
    x : Integer := 10;
    y : Integer := 0;
    result : Integer;
begin
    try
        if y = 0 then
            raise Constraint_Error;
        end if;
        result := x / y;
    exception
        when Constraint_Error =>
            result := 0;
        when others =>
            result := -1;
    end;
    Put(result);
    New_Line;
end exception_handling;
