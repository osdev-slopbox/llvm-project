-- Test: heap allocation with new (P10 GNAT compatibility)
-- RUN: %clang_ada -a2012 -o %t %s %ada_link_flags
-- RUN: %t
-- CHECK: 42
-- CHECK: 3.14
-- CHECK: Hello
-- CHECK: 10
-- CHECK: 20

procedure storage_heap is
    use Ada.Text_IO;
    use Ada.Integer_Text_IO;

    P : access Integer := new Integer(42);
    F : access Float := new Float(3.14);
    S : access String := new String("Hello");
begin
    print_int(P.all);
    New_Line;
    print_float(F.all);
    New_Line;
    Put(S.all);
    New_Line;

    -- Access type arithmetic
    declare
        Arr : access array(1..10) of Integer := new array(1..10) of Integer(0);
    begin
        Arr.all(1) := 10;
        Arr.all(2) := 20;
        print_int(Arr.all(1));
        New_Line;
        print_int(Arr.all(2));
        New_Line;
    end;
end storage_heap;