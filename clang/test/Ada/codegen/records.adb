// RUN: %clang_ada -a2012 -c %s -o %t.o | FileCheck %s

procedure UsePoint is
    type Point is
        record
            X : Integer;
            Y : Integer;
        end record;
    P : Point;
begin
    P.X := 10;
    P.Y := 20;
end UsePoint;

CHECK: define void @UsePoint
CHECK: %P = alloca {{.*}}, align
CHECK: getelementptr {{.*}}, ptr %P, i32 0, i32 0
CHECK: store i32 10, ptr %{{.*}}, align 4
CHECK: getelementptr {{.*}}, ptr %P, i32 0, i32 1
CHECK: store i32 20, ptr %{{.*}}, align 4
