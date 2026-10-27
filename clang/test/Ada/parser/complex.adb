// RUN: %clang_ada -fsyntax-only %s

procedure Test_Complex is
    X, Y : Integer := 10;
    Z : Real;

    procedure Inner is
    begin
        null;
    end Inner;

begin
    if X > Y then
        Z := 1.0;
    else
        Z := 2.0;
    end if;

    while X > 0 loop
        X := X - 1;
        exit when X = 0;
    end loop;

    for I in 1 .. 10 loop
        null;
    end loop;
end Test_Complex;