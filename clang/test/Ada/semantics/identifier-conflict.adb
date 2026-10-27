-- Test: identifier conflict
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure identifier_conflict is
    x : Integer := 5;
    procedure foo is
    begin
        null;
    end foo;
begin
    null;
end identifier_conflict;
