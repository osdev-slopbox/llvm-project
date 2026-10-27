// RUN: %clang_ada --driver-mode=ada -x ada %s -fsyntax-only 2>&1 | FileCheck %s
-- Test accept statement parsing
task TaskWithEntry is
    entry DoWork(X : Integer);
end TaskWithEntry;

task body TaskWithEntry is
begin
    accept DoWork(X : Integer) do
        null;
    end DoWork;
end TaskWithEntry;

procedure Main is
begin
    null;
end Main;
// CHECK: ac1: parsed unit: yes
