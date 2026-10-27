-- RM 5.1, 5.2, 5.3, 5.4 Control flow
-- RUN: %clang_ada -a2012 -o %t %s %ada_link_flags
-- RUN: %t
-- CHECK: 1
-- CHECK: 2
-- CHECK: 3
-- CHECK: done

procedure ControlFlow is
    use Ada.Integer_Text_IO;
    use Ada.Text_IO;

    procedure CountTo(N : Integer) is
    begin
        declare
            I : Integer := 1;
        begin
            while I <= N loop
                print_int(I);
                New_Line;
                I := I + 1;
            end loop;
        end;
    end CountTo;
begin
    CountTo(3);
    Put("done");
    New_Line;
end ControlFlow;