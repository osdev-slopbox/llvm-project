-- Test: aspect inline
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure aspect_inline is
    procedure foo with Inline is
    begin
        null;
    end foo;
begin
    foo;
end aspect_inline;
