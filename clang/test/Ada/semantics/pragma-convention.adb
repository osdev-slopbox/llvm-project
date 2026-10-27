-- Test: Optimization pragma (GNAT compatibility)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure pragma_optimize is
    pragma Optimize;
begin
    null;
end pragma_optimize;