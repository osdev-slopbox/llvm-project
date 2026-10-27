// RUN: %clang_ada -a2012 -c %s -o %t.o | FileCheck %s

procedure UseIf is
    X : Integer := 10;
begin
    if X > 5 then
        X := 1;
    else
        X := 2;
    end if;
end UseIf;

CHECK: %X = alloca i32, align 4
CHECK: store i32 10, ptr %X, align 4
CHECK: %cmp = icmp sgt i32 %{{.*}}, 5
CHECK: br i1 %cmp, label %if.then, label %if.else
CHECK: if.then:
CHECK: store i32 1, ptr %X, align 4
CHECK: if.else:
CHECK: store i32 2, ptr %X, align 4
