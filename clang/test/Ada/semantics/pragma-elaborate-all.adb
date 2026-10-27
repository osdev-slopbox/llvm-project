-- Test: Elaborate_All pragma (GNAT compatibility)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

with Elaborated_Package;
pragma Elaborate_All (Elaborated_Package);

procedure pragma_elaborate_all is
begin
    null;
end pragma_elaborate_all;