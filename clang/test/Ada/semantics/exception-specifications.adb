-- Test: exception specifications
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure exception_specs is
    x : Integer;
begin
    try
        x := 1;
    exception
        when e : Exception =>
            null;
    end;
end exception_specs;
