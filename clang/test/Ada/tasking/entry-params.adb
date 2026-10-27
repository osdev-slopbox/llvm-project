-- Test: entry with parameters
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure entry_params is
    task T is
        entry Process(x : Integer);
    end T;

    task body T is
    begin
        loop
            accept Process(x : Integer) do
                null;
            end Process;
        end loop;
    end T;
begin
    T.Process(42);
end entry_params;
