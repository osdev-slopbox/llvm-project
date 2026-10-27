-- Test: protected entry
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure protected_entry is
    protected P is
        entry Do_Work when (true);
    end P;

    protected body P is
        entry Do_Work when (true) is
        begin
            null;
        end Do_Work;
    end P;
begin
    P.Do_Work;
end protected_entry;
