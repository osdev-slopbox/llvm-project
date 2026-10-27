add_clang_library(clangAda
  AdaFrontend.cpp
  Lexer/AdaLexer.cpp
  Parser/AdaParser.cpp
  Frontend/AdaFrontendAction.cpp
  AST/AST.cpp
  AST/Expressions.cpp
  Semantics/SemanticAnalyzer.cpp
  CodeGen/AdaCodeGen.cpp
  Serialization/ModuleFile.cpp

  LINK_LIBS
  clangBasic
  )

# Minimal in-tree Ada runtime (D4). Built as a static library so the
# driver can auto-link it via "-lclangAdaRuntime" (see Gnu.cpp link hook).
add_library(clangAdaRuntime STATIC
  Runtime/ada_runtime.c
  )
