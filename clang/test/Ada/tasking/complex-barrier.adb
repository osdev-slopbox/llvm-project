-- Test: complex barrier
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_barrier is
    count : Integer := 0;
    task T is
        entry Ready when (count >= 2);
    end T;

    task body T is
    begin
        loop
            accept Ready do
                null;
            end Ready;
        end loop;
    end T;
begin
    null;
end complex_barrier;
