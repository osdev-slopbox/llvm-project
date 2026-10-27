// RUN: %clang_ada --driver-mode=ada -x ada %s -fsyntax-only 2>&1 | FileCheck %s
-- Test task declaration parsing
task SimpleTask is
begin
end SimpleTask;

procedure Main is
begin
    null;
end Main;
// CHECK: ac1: parsed unit: yes
