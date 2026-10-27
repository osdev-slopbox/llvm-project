-- Test: access type operations
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure access_operations is
    type IntPtr is access Integer;
    p : IntPtr;
    x : Integer := 10;
begin
    p := new Integer'(5);
    x := p.all;
    deallocate(p);
end access_operations;
