// RUN: %clang_ada -fsyntax-only %s

package My_Package is
    type My_Type is private;

    function Create return My_Type;
    procedure Destroy(X : in out My_Type);

private
    type My_Type is record
        Value : Integer;
    end record;
end My_Package;