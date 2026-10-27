// RUN: %clang_ada -fsyntax-only -a2012 %s
-- Simple program that should pass semantic analysis
procedure Hello is
begin
    null;
end Hello;
