-- Test: terminate alternative
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure terminate_alternative is
    task T is
        entry Work;
    end T;

    task body T is
    begin
        loop
            select
                accept Work do
                    null;
                end Work;
            or
                exit;
            end select;
        end loop;
    end T;
begin
    null;
end terminate_alternative;
