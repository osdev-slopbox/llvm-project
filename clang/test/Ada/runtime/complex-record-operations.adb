-- Test: record operations
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_record_operations is
    type Point is record
        x : Integer;
        y : Integer;
    end record;

    p : Point;
begin
    p.x := 3;
    p.y := 4;
    print_int(p.x);
    new_line;
end complex_record_operations;

-- CHECK: 3
