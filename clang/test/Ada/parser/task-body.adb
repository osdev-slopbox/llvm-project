// RUN: %clang_ada --driver-mode=ada -x ada %s -fsyntax-only 2>&1 | FileCheck %s
-- Test task body parsing
task SimpleTask is
begin
    null;
end SimpleTask;

task body SimpleTask is
begin
    null;
end SimpleTask;

procedure Main is
begin
    null;
end Main;
// CHECK: ac1: parsed unit: yes
