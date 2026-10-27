// RUN: %clang_ada --driver-mode=ada -x ada %s -fsyntax-only 2>&1 | FileCheck %s
-- Test task with entry declarations
task TaskWithEntry is
    entry DoWork(X : Integer);
end TaskWithEntry;

procedure Main is
begin
    null;
end Main;
// CHECK: ac1: parsed unit: yes
