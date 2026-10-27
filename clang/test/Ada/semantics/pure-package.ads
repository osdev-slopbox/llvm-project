-- Test: Pure package spec
-- RUN: %clang_ada -a2012 -fsyntax-only %s

pragma Pure;

package Pure_Package is
    function Add(a : Integer; b : Integer) return Integer;
end Pure_Package;