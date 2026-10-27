-- Test: void function (procedure)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure void_function is
    procedure do_work is
    begin
        null;
    end do_work;
begin
    do_work;
end void_function;
