-- Test: variadic-like parameter passing
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure variadic_args is
    procedure process(a : Integer; b : Integer; c : Integer; d : Integer) is
    begin
        null;
    end process;
begin
    process(1, 2, 3, 4);
end variadic_args;
