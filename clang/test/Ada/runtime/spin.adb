// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: sh -c 'timeout 5 %t_prog; test $? = 124'

procedure Spin is
begin
  loop
    null;
  end loop;
end Spin;
