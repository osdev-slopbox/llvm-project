-- Test: Pure pragma (GNAT compatibility)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

with Pure_Package;
procedure pragma_pure is
begin
    null;
end pragma_pure;