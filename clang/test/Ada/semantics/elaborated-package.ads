-- Test: Elaborated package
-- RUN: %clang_ada -a2012 -fsyntax-only %s

package Elaborated_Package is
    Y : Integer := 10;
end Elaborated_Package;