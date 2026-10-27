-- Test: asynchronous select
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure async_select is
    task T is
        entry Do_Work;
    end T;

    task body T is
    begin
        loop
            select
                accept Do_Work do
                    null;
                end Do_Work;
            or
                null;
            end select;
        end loop;
    end T;
begin
    null;
end async_select;
