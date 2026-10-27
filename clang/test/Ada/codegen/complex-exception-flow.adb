-- Test: complex exception flow
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_exception_flow is
    procedure risky is
    begin
        raise Constraint_Error;
    end risky;
begin
    try
        risky;
    exception
        when Constraint_Error =>
            null;
    end;
end complex_exception_flow;
