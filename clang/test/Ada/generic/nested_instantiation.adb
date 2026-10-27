// RUN: %clang_ada -fsyntax-only -a2012 %s
// Nested generic instantiation (RM 12.5.3)

generic
   type T is private;
function Identity(X : T) return T;

function Identity(X : T) return T is
begin
   return X;
end Identity;

-- Outer generic
generic
   type U is private;
procedure Outer(X : U);

procedure Outer(X : U) is
   -- Nested instantiation
   function Local_Identity is new Identity(T => U);
begin
   null;
end Outer;
