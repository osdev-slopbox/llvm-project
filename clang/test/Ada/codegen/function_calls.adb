// RUN: %clang_ada -a2012 -c %s -o %t.o | FileCheck %s

procedure Main is
    function Max(A : Integer; B : Integer) return Integer is
    begin
        if A > B then
            return A;
        else
            return B;
        end if;
    end Max;
    R : Integer;
begin
    R := Max(5, 10);
end Main;

CHECK: define void @Main
CHECK: %R = alloca i32, align 4
CHECK: %{{[0-9]+}} = call i32 @Max(i32 5, i32 10)
CHECK: store i32 %{{[0-9]+}}, ptr %R, align 4
CHECK: define i32 @Max(i32 %A, i32 %B)
CHECK: %cmp = icmp sgt i32 %A, %B
CHECK: br i1 %cmp, label %if.then, label %if.else
CHECK: if.then:
CHECK: ret i32 %A
CHECK: if.else:
CHECK: ret i32 %B
