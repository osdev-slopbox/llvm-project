// RUN: %clang_ada -a2012 -fsyntax-only %s

function Add(A, B : Integer)
   return Integer
with Pre => A >= 0 and B >= 0,
     Post => Add'Result >= 0 is
begin
   return A + B;
end Add;

procedure ContractTest is
    Result : Integer;
begin
    Result := Add(5, 3);
end ContractTest;
