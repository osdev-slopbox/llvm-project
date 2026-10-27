// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: timeout 5 %t_prog

procedure LoopExit is
   Count : Integer := 0;
begin
  loop
    Count := Count + 1;
    exit when Count = 10;
  end loop;
end LoopExit;
