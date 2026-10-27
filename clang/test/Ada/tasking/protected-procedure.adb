-- Test: protected procedure
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure protected_procedure is
    protected P is
        procedure Do_Work;
    end P;

    protected body P is
        procedure Do_Work is
        begin
            null;
        end Do_Work;
    end P;
begin
    P.Do_Work;
end protected_procedure;
