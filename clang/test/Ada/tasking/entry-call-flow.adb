// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 -c %s -o %t.o
// RUN: gcc -o %t %t.o -L/usr/local/src/slopbox/llvm/build/lib -lclangAdaRuntime -lc -lpthread
// RUN: %t
-- Test full entry call/accept flow
task Producer is
    entry Send(Value : Integer);
end Producer;

task body Producer is
begin
    accept Send(Value : Integer) do
        null;
    end Send;
end Producer;

procedure Main is
begin
    Producer.Send(42);
end Main;
