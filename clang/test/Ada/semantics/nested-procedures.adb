-- Test: nested procedure declarations
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure nested_procedures is
    procedure inner(x : Integer) is
    begin
        null;
    end inner;

    procedure outer is
    begin
        inner(5);
    end outer;
begin
    outer;
end nested_procedures;
