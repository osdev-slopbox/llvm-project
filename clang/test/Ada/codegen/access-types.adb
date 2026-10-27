-- Test: access types (pointers)
-- RUN: %clang_ada -a2012 -c %s -o /dev/null

procedure access_types is
    type IntPtr is access Integer;
    p : IntPtr;
    x : Integer := 42;
    y : Integer;
begin
    p := new Integer'(100);
    y := p.all;
    deallocate(p);
    p := x'access;
    y := p.all;
end access_types;
