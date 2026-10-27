-- Test: barrier synchronization
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure barrier is
    task T is
        entry Ready;
        entry Go;
    end T;

    task body T is
    begin
        accept Ready;
        null;
        accept Go;
    end T;
begin
    null;
end barrier;
