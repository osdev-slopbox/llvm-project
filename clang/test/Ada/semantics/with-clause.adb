// RUN: %clang_ada -a2012 -fsyntax-only %s

with Ada.Text_IO;
use Ada.Text_IO;

procedure WithClause is
    X : Integer := 42;
begin
    null;
end WithClause;
