-- Test: complex aspect
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_aspect is
    function foo(x : Integer) return Integer with Inline, Pure is
    begin
        return x * 2;
    end foo;
begin
    null;
end complex_aspect;
