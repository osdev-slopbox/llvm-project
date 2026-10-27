-- Test: renaming
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure renaming is
    x : Integer := 5;
    y : Integer renames x;
begin
    null;
end renaming;
