-- Test: Priority pragma (GNAT compatibility)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure pragma_priority is
    task T is
        pragma Priority (1);
    end T;

    task body T is
    begin
        null;
    end T;
begin
    null;
end pragma_priority;