-- Test: default parameter values
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure default_params is
    procedure greet(name : String; msg : String := "Hello") is
    begin
        null;
    end greet;
begin
    greet("Alice");
    greet("Bob", "Hi");
end default_params;
