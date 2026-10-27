-- Test: Preelaborated pragma (GNAT compatibility)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

pragma Preelaborated;

package Preelaborated_Package is
    X : Integer := 42;
end Preelaborated_Package;