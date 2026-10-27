// RUN: %clang_ada -a2012 -c %s -o %t.o | FileCheck %s

procedure TryTest is
    X : Integer := 5;
begin
    try
        X := X + 1;
        raise Constraint_Error;
    exception
        when Constraint_Error is
            X := -1;
        when others is
            X := -2;
    end try;
end TryTest;

CHECK: @[[H1:[0-9]+]] = private unnamed_addr constant [17 x i8] c"Constraint_Error\00", align 1
CHECK: @[[H2:[0-9]+]] = private unnamed_addr constant [17 x i8] c"Constraint_Error\00", align 1
CHECK: @llvm.global_ctors = appending global [1 x { i32, ptr, ptr }] [{ i32, ptr, ptr } { i32 65535, ptr @__ada_ctor_TryTest, ptr null }]
CHECK: define void @TryTest()
CHECK: store i32 5, ptr %X, align 4
CHECK: %try.h = call ptr @__ada_push_handler()
CHECK: %try.r = call i32 @__ada_setctx(ptr %try.h)
CHECK: %[[R:.*]] = icmp ne i32 %try.r, 0
CHECK: br i1 %[[R]], label %try.dispatch, label %try.body
CHECK: try.body:
CHECK: call void @__ada_raise(ptr @[[H1]])
CHECK: call void @__ada_pop_handler()
CHECK: br label %try.end
CHECK: try.dispatch:
CHECK: %try.match = call i32 @__ada_exception_match(ptr @[[H2]])
CHECK: br i1 %{{.*}}, label %try.handler0, label %try.match1
CHECK: try.end:
CHECK: ret void
CHECK: try.match1:
CHECK: br label %try.handler1
CHECK: try.handler0:
CHECK: store i32 -1, ptr %X, align 4
CHECK: br label %try.end
CHECK: try.handler1:
CHECK: store i32 -2, ptr %X, align 4
CHECK: try.reraise:
CHECK: %try.name = call ptr @__ada_current_exception()
CHECK: call void @__ada_raise(ptr %try.name)
CHECK: unreachable
CHECK: define internal void @__ada_ctor_TryTest()
CHECK: call void @__ada_main_register(ptr @TryTest)
