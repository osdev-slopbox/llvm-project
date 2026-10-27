-- Test: Elaborate pragma (GNAT compatibility)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

with Elaborated_Package;
pragma Elaborate (Elaborated_Package);

procedure pragma_elaborate is
begin
    null;
end pragma_elaborate;