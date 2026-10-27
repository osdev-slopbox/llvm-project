// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 -c %s -o %t.o
// RUN: gcc -o %t %t.o -L/usr/local/src/slopbox/llvm/build/lib -lclangAdaRuntime -lc -lpthread
// RUN: %t
-- Simple task test
task SimpleTask is
begin
end SimpleTask;

task body SimpleTask is
begin
    null;
end SimpleTask;

procedure Main is
begin
    null;
end Main;
