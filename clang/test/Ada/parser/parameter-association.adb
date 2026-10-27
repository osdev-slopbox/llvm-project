-- Test: parameter association
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure parameter_association is
    procedure foo(a : Integer; b : Integer) is
    begin
        null;
    end foo;
begin
    foo(a => 1, b => 2);
    foo(b => 2, a => 1);
end parameter_association;
