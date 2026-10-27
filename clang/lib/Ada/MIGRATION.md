# Migrating from GNAT to clang-ada

This guide helps you migrate your Ada 2012 programs from GNAT to clang-ada.

## Command-Line Differences

| GNAT | clang-ada | Notes |
|------|-----------|-------|
| `gnatmake foo.adb` | `clang-ada -a2012 -c foo.adb -o foo.o && clang-ada -o foo foo.o` | Manual two-step build |
| `gnatmake -O2` | `clang-ada -a2012 -O2 -c foo.adb -o foo.o` | Same optimization level |
| `gnatmake -g` | `clang-ada -a2012 -g -c foo.adb -o foo.o` | Same debug level |
| `gnatmake -w` | `clang-ada -a2012 -Wall -c foo.adb -o foo.o` | Same warning level |
| `gnatmake -E` | `clang-ada -a2012 -fsyntax-only foo.adb` | Syntax check only |
| `gnatmake -c` | `clang-ada -a2012 -c foo.adb -o foo.o` | Compile only |
| `gnatmake -f` | N/A | No forced rebuild |
| `gnatmake -p` | N/A | No project file support |

## Pragma Differences

Most standard Ada 2012 pragmas are supported. However, some GNAT-specific pragmas are not:

| Pragma | GNAT | clang-ada |
|--------|------|-----------|
| Inline | Yes | Yes |
| Export | Yes | Yes |
| Import | Yes | Yes |
| Pack | Yes | Yes |
| Suppress | Yes | Yes |
| Check | Yes | Yes |
| Pure | Yes | Yes |
| Preelaborated | Yes | Yes |
| Elaborate | Yes | Yes |
| Elaborate_All | Yes | Yes |
| Priority | Yes | Yes |
| Link_Name | Yes | Yes |
| Convention | Yes | Yes |
| At | Yes | Yes |
| Address | Yes | Yes |
| Storage_Size | Yes | Yes |
| Alignment | Yes | Yes |
| Warnings | Yes | Yes |
| Style_Checks | Yes | Yes |
| Optimize | Yes | Yes |
| Inline_All_Calls | Yes | Yes |
| Debug | Yes | Yes |
| Assert | Yes | Yes |
| Interrupt_Priority | Yes | Yes |
| Protected | Yes | Yes |
| Asynchronous | Yes | Yes |
| Asynchronous_Select | Yes | Yes |
| Terminated | Yes | Yes |
| GNAT-specific | Yes | No |

## Runtime Library Differences

The clang-ada runtime library is minimal compared to GNAT's libgnat. Some packages are not available:

| Package | GNAT | clang-ada |
|---------|------|-----------|
| Ada.Text_IO | Yes | Yes (minimal) |
| Ada.Integer_Text_IO | Yes | Yes (minimal) |
| Ada.Float_Text_IO | Yes | Yes (minimal) |
| Ada.Exceptions | Yes | Yes (minimal) |
| Ada.Tags | Yes | Yes (minimal) |
| Ada.Strings | Yes | No |
| Ada.Containers | Yes | No |
| Ada.Calendar | Yes | No |
| Ada.Directories | Yes | No |
| Ada.Environment_Variables | Yes | No |
| Ada.Finalization | Yes | Yes (minimal) |
| Ada.Task_Identification | Yes | No |
| Ada.Synchronous_Task_Control | Yes | No |

## Build System Differences

GNAT uses `gnatmake` as its build driver. clang-ada uses a Make-first approach:

```make
# GNAT-style build
gnatmake -O2 -g foo.adb bar.adb

# clang-ada build
clang-ada -a2012 -O2 -g -c foo.adb -o foo.o -MD -MF foo.d
clang-ada -a2012 -O2 -g -c bar.adb -o bar.o -MD -MF bar.d
clang-ada -o program foo.o bar.o
```

With Makefile dependency generation:
```make
CXX = clang-ada
CXXFLAGS = -a2012 -O2 -g
OBJS = foo.o bar.o

program: $(OBJS)
	$(CXX) -o $@ $(OBJS)

%.o: %.adb
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MF $*.d

-include $(OBJS:.o=.d)
```

## Performance Comparison

clang-ada uses LLVM's code generator, which produces highly optimized machine code.
Performance should be comparable to GNAT for most programs.

## Debugging

clang-ada generates DWARF debug information, which is compatible with GDB and LLDB.

```sh
clang-ada -a2012 -g -c foo.adb -o foo.o
gdb ./program
```

## Troubleshooting

### "error: undeclared identifier"

Make sure you have the correct `with` clause for the package.

### "error: cannot resolve call to"

Check the subprogram signature and arguments. Ada has strict type checking.

### "error: elaboration dependency not declared"

Make sure the package referenced in the `Elaborate` pragma is declared.

### Linking errors

If you get linking errors, try using `gcc` as the linker:
```sh
clang-ada -a2012 -c foo.adb -o foo.o
gcc -o program foo.o -L./build/lib -lclangAdaRuntime -lc
```

## Next Steps

- Review the [User Manual](README.md) for detailed documentation
- Check the [Examples](examples/) directory for sample programs
- Run the test suite to verify your installation