// RUN: %clang_ada -fsyntax-only %s

generic
    type Element is private;
function Max(A, B : Element) return Element;

function Max(A, B : Element) return Element is
begin
    if A > B then
        return A;
    else
        return B;
    end if;
end Max;