-- Test: visibility rules
-- RUN: %clang_ada -a2012 -fsyntax-only %s

with Ada.Text_IO; use Ada.Text_IO;

procedure visibility is
    x : Integer := 5;
    y : Integer := 10;
begin
    Put(x);
end visibility;
