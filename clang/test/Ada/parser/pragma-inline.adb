-- Test: pragma inline
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure pragma_inline is
    procedure foo is
        pragma Inline(foo);
    begin
        null;
    end foo;
begin
    foo;
end pragma_inline;
