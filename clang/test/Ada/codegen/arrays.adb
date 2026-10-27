-- Test: array operations
-- RUN: %clang_ada -a2012 -c %s -o /dev/null

procedure arrays is
    type IntArray is array (1..10) of Integer;
    arr : IntArray;
    i : Integer;
    sum : Integer := 0;
begin
    for i in 1..10 loop
        arr(i) := i * 2;
        sum := sum + arr(i);
    end loop;
    Put(sum);
    New_Line;
end arrays;
