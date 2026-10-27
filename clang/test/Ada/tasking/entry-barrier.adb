-- Test: entry barrier
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure entry_barrier is
    ready : Boolean := false;
    task T is
        entry Work when (ready);
    end T;

    task body T is
    begin
        loop
            accept Work do
                null;
            end Work;
        end loop;
    end T;
begin
    null;
end entry_barrier;
