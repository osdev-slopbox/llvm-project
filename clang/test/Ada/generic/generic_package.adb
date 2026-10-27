// RUN: %clang_ada -fsyntax-only -a2012 %s
// Generic package (RM 12.2.1)
// A generic package with a formal type parameter.

generic
   type Element is private;
package Container is
   procedure Add(X : in Element);
   function Get return Element;
end Container;

-- Instantiation with Integer
package Int_Container is new Container(Element => Integer);
