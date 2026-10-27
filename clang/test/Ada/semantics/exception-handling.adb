-- Test: exception handling
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure exception_handling is
    x : Integer := 0;
begin
    try
        if x = 0 then
            raise Constraint_Error;
        end if;
    exception
        when Constraint_Error =>
            x := 1;
    end;
end exception_handling;
