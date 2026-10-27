-- Test: complex pragma
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_pragma is
    pragma Inline_All_Calls;
begin
    null;
end complex_pragma;
