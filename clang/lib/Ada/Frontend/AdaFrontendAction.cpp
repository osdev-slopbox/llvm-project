//===--- AdaFrontendAction.cpp - Ada 2012 frontend action impl --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/Frontend/AdaFrontendAction.h"
#include "clang/Ada/Lexer/AdaLexer.h"
#include "clang/Ada/Parser/AdaParser.h"
#include "clang/Ada/Semantics/SemanticAnalyzer.h"
#include "clang/Ada/CodeGen/AdaCodeGen.h"
#include "clang/Ada/Serialization/ModuleFile.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace clang {
namespace ada {

// Global diagnostics instance
static Diagnostics GlobalDiagnostics;

Diagnostics &GetDiagnostics() {
  return GlobalDiagnostics;
}

// Read file contents
static std::string ReadFile(const std::string &Path) {
  std::ifstream F(Path, std::ios::binary);
  if (!F) {
    return "";
  }
  std::stringstream SS;
  SS << F.rdbuf();
  return SS.str();
}

// Write file contents
static bool WriteFile(const std::string &Path, const std::string &Contents) {
  std::ofstream F(Path, std::ios::binary);
  if (!F) {
    return false;
  }
  F << Contents;
  F.flush();
  F.close();
  return true;
}

// Check if argument matches a flag
static bool MatchFlag(const char *Arg, const char *Flag) {
  return strcmp(Arg, Flag) == 0;
}

// Get value for flag (next argument)
static bool GetFlagValue(const char *Arg, const char *Flag, const char **Value) {
  if (strncmp(Arg, Flag, strlen(Flag)) == 0 && Arg[strlen(Flag)] == '=') {
    *Value = Arg + strlen(Flag) + 1;
    return true;
  }
  return false;
}

// Map a warning name (the part after -Wada- / -Wno-ada-) to a WarningKind.
// Returns false if the name is unknown.
static bool LookupWarningKind(const char *Name, WarningKind &Kind) {
  struct Entry { const char *Name; WarningKind Kind; };
  static const Entry Table[] = {
      {"unknown-pragmas", WarningKind::UnknownPragma},
      {"unknown-aspects", WarningKind::UnknownAspect},
      {"unused-variable", WarningKind::UnusedVariable},
      {"unused-parameter", WarningKind::UnusedParameter},
      {"unreachable-code", WarningKind::UnreachableCode},
      {"unused-with", WarningKind::UnusedWith},
      {"type-limits", WarningKind::ComparisonAlways},
      {"shadow", WarningKind::Shadowing},
      {"pedantic", WarningKind::Pedantic},
      {"deprecated", WarningKind::Deprecated},
      {"invalid-pragma-value", WarningKind::InvalidPragmaValue},
  };
  for (const auto &E : Table) {
    if (strcmp(Name, E.Name) == 0) {
      Kind = E.Kind;
      return true;
    }
  }
  return false;
}

// Handle -Wada-<name> (Enable=true) and -Wno-ada-<name> (Enable=false).
// `Name` is the suffix after the prefix; `FullArg` is the original argv entry
// (used verbatim in the unknown-option diagnostic).
static void ParseWarningToggle(const char *Name, const char *FullArg,
                               bool Enable) {
  WarningKind Kind;
  if (!LookupWarningKind(Name, Kind)) {
    GetDiagnostics().WarnAlways(std::string("unknown warning option '") +
                                FullArg + "'");
    return;
  }
  if (Enable)
    GetDiagnostics().EnableWarning(Kind);
  else
    GetDiagnostics().DisableWarning(Kind);
}

bool ParseAc1Args(int Argc, const char * const *Argv, AdaFrontendOptions &Opts) {
  bool HasDepScan = false;
  bool HasSyntaxOnly = false;

  std::cerr << "ac1: parsing " << Argc << " args\n";
  for (int i = 0; i < Argc; i++) {
    std::cerr << "ac1: arg[" << i << "] = " << Argv[i] << "\n";
  }

  for (int i = 1; i < Argc; i++) {
    const char *Arg = Argv[i];

    if (MatchFlag(Arg, "-fsyntax-only")) {
      HasSyntaxOnly = true;
    } else if (MatchFlag(Arg, "-emit-obj")) {
      Opts.Action = AdaActionKind::Compile;
    } else if (MatchFlag(Arg, "-emit-ada-preprocessed")) {
      Opts.Action = AdaActionKind::EmitPreprocessed;
    } else if (MatchFlag(Arg, "-ast-dump")) {
      Opts.Action = AdaActionKind::ASTDump;
    } else if (MatchFlag(Arg, "-M") || MatchFlag(Arg, "-MD") ||
               MatchFlag(Arg, "-MMD")) {
      HasDepScan = true;
      Opts.EmitDepfile = true;
    } else if (MatchFlag(Arg, "-MF")) {
      if (i + 1 < Argc) {
        Opts.Depfile = Argv[++i];
      }
    } else if (strncmp(Arg, "-MF=", 4) == 0) {
      Opts.Depfile = Arg + 4;
    } else if (MatchFlag(Arg, "-MT")) {
      if (i + 1 < Argc) {
        Opts.DepTarget = Argv[++i];
      }
    } else if (strncmp(Arg, "-MT=", 4) == 0) {
      Opts.DepTarget = Arg + 4;
    } else if (MatchFlag(Arg, "-MQ")) {
      if (i + 1 < Argc) {
        Opts.DepTarget = Argv[++i];
      }
    } else if (strncmp(Arg, "-MQ=", 4) == 0) {
      Opts.DepTarget = Arg + 4;
    } else if (MatchFlag(Arg, "-quiet") || MatchFlag(Arg, "-q")) {
      Opts.Quiet = true;
    } else if (MatchFlag(Arg, "-o")) {
      if (i + 1 < Argc) {
        Opts.OutputFile = Argv[++i];
      }
    } else if (MatchFlag(Arg, "-Werror")) {
      GetDiagnostics().SetWerrorAll(true);
    } else if (MatchFlag(Arg, "-Wall")) {
      GetDiagnostics().SetWarningLevel(WarningLevel::All);
    } else if (MatchFlag(Arg, "-Wextra")) {
      GetDiagnostics().SetWarningLevel(WarningLevel::Extra);
    } else if (MatchFlag(Arg, "-Wpedantic")) {
      GetDiagnostics().SetWarningLevel(WarningLevel::Pedantic);
    } else if (MatchFlag(Arg, "-w")) {
      GetDiagnostics().SetWarningLevel(WarningLevel::None);
    } else if (strncmp(Arg, "-Wno-ada-", 9) == 0) {
      ParseWarningToggle(Arg + 9, Arg, /*Enable=*/false);
    } else if (strncmp(Arg, "-Wada-", 6) == 0) {
      ParseWarningToggle(Arg + 6, Arg, /*Enable=*/true);
    } else if (MatchFlag(Arg, "-fno-ada-contracts")) {
      Opts.EnableContracts = false;
    } else if (MatchFlag(Arg, "-O0")) {
      Opts.OptimizationLevel = 0;
    } else if (MatchFlag(Arg, "-O1")) {
      Opts.OptimizationLevel = 1;
    } else if (MatchFlag(Arg, "-O2")) {
      Opts.OptimizationLevel = 2;
    } else if (MatchFlag(Arg, "-O3")) {
      Opts.OptimizationLevel = 3;
    } else if (MatchFlag(Arg, "-O")) {
      Opts.OptimizationLevel = 2; // -O defaults to -O2 (like GCC/Clang)
    } else if (strncmp(Arg, "-original-source=", 17) == 0) {
      Opts.OriginalSourceFile = Arg + 17;
    } else if (Arg[0] != '-') {
      // Input file
      Opts.InputFile = Arg;
    }
  }

  // Action priority: dep scan > explicit emit actions > syntax-only
  if (HasDepScan) {
    Opts.Action = AdaActionKind::DependencyScan;
  } else if (Opts.Action == AdaActionKind::EmitPreprocessed) {
    // Keep EmitPreprocessed as-is
  } else if (HasSyntaxOnly) {
    Opts.SyntaxOnly = true;
    Opts.Action = AdaActionKind::SyntaxOnly;
  }

  return true;
}

// Extract with-clause dependencies from source
static std::vector<std::string> ExtractWithClauseDeps(const std::string &Source) {
  std::vector<std::string> Deps;
  AdaLexer Lexer("source", Source);
  Token Tok = Lexer.NextToken();

  while (Tok.Kind != TokKind::EOF_Tok) {
    if (Tok.Kind == TokKind::Keyword && Tok.Text == "with") {
      Tok = Lexer.NextToken();
      while (Tok.Kind == TokKind::Identifier) {
        std::string PkgName = Tok.Spelling;
        // Convert to lowercase for dependency name
        std::string Lower = PkgName;
        std::transform(Lower.begin(), Lower.end(), Lower.begin(), ::tolower);
        Deps.push_back(Lower);
        Tok = Lexer.NextToken();
        if (Tok.Kind == TokKind::Comma) {
          Tok = Lexer.NextToken();
        } else {
          break;
        }
      }
    }
    Tok = Lexer.NextToken();
  }

  return Deps;
}

// Emit dependency file in make format
static int EmitDependencyFile(const std::string &Target,
                               const std::vector<std::string> &Deps,
                               const std::string &Depfile) {
  if (Depfile.empty()) {
    // stdout mode
    std::cout << Target << ": ";
    for (size_t i = 0; i < Deps.size(); i++) {
      if (i > 0) std::cout << " ";
      std::cout << Deps[i] << ".ads";
    }
    std::cout << "\n";
    return 0;
  }

  // File mode
  std::ofstream F(Depfile);
  if (!F) {
    return 1;
  }

  F << Target << ": ";
  for (size_t i = 0; i < Deps.size(); i++) {
    if (i > 0) F << " ";
    F << Deps[i] << ".ads";
  }
  F << "\n";
  F.close();
  return 0;
}

int RunFrontendAction(const AdaFrontendOptions &Opts) {
  if (Opts.InputFile.empty()) {
    std::cerr << "ac1: error: no input file\n";
    return 1;
  }

  // Read source file
  std::string Source = ReadFile(Opts.InputFile);
  if (Source.empty()) {
    std::cerr << "ac1: error: could not read input file '" << Opts.InputFile << "'\n";
    return 1;
  }

  switch (Opts.Action) {
    case AdaActionKind::SyntaxOnly: {
      std::cerr << "ac1: running syntax-only analysis\n";
      // Parse and report errors
      AdaParser Parser(Opts.InputFile, Source);
      auto Unit = Parser.ParseLibraryUnit();
      std::cerr << "ac1: parsed unit: " << (Unit ? "yes" : "no") << "\n";
      if (Unit) {
        // Run semantic analysis
        std::cerr << "ac1: running semantic analysis\n";
        SemanticAnalyzer Analyzer;
        bool SemResult = Analyzer.AnalyzeLibraryUnit(*Unit);
        std::cerr << "ac1: semantic analysis result: " << (SemResult ? "ok" : "error") << "\n";
        // -Werror promotes warnings to errors: HasErrors() is then true even
        // when the semantic analyzer itself succeeded.
        if (!SemResult || GetDiagnostics().HasErrors()) {
          std::cerr << "ac1: error: semantic analysis failed\n";
          return 1;
        }
        return 0;
      }
      return 1;
    }

    case AdaActionKind::DependencyScan: {
      // Extract with-clause dependencies
      auto Deps = ExtractWithClauseDeps(Source);

      // Determine target name from input file
      std::string Target = Opts.DepTarget;
      if (Target.empty()) {
        // Derive from input file name
        std::string Base = Opts.InputFile;
        size_t Slash = Base.rfind('/');
        if (Slash != std::string::npos) {
          Base = Base.substr(Slash + 1);
        }
        size_t Dot = Base.rfind('.');
        if (Dot != std::string::npos) {
          Target = Base.substr(0, Dot) + ".o";
        } else {
          Target = Base + ".o";
        }
      }

      if (EmitDependencyFile(Target, Deps, Opts.Depfile) != 0) {
        return 1;
      }

      // If an output file is specified, also write the preprocessed output
      // (for the driver's temp file pipeline).
      if (!Opts.OutputFile.empty()) {
        if (!WriteFile(Opts.OutputFile, Source)) {
          std::cerr << "ac1: error: could not write output file '"
                    << Opts.OutputFile << "'\n";
          return 1;
        }
      }

      return 0;
    }

    case AdaActionKind::ASTDump: {
      // Parse and dump AST
      AdaParser Parser(Opts.InputFile, Source);
      auto Unit = Parser.ParseLibraryUnit();
      if (Unit) {
        std::cout << "LibraryUnit: " << Unit->Name << "\n";
        for (const auto &W : Unit->WithClauses)
          std::cout << "  WithClause: " << W << "\n";
        if (Unit->Root) Unit->Root->dump();
        return 0;
      }
      return 1;
    }

    case AdaActionKind::Compile: {
      std::cerr << "ac1: running code generation\n";

      // Check for module files for with-clause dependencies
      // For P7, we load module files for any with-ed specs
      // and merge their declarations into the compilation
      std::vector<std::string> WithDeps = ExtractWithClauseDeps(Source);
      std::vector<std::unique_ptr<LibraryUnit>> LoadedModules;

      // Use OriginalSourceFile for module file lookup (if available)
      std::string SourceFile = Opts.OriginalSourceFile.empty() ? Opts.InputFile : Opts.OriginalSourceFile;

      for (const auto &Dep : WithDeps) {
        // Look for module file in the same directory as the source file
        std::string BaseDir = SourceFile.substr(0, SourceFile.rfind('/'));
        if (BaseDir.empty()) BaseDir = ".";

        std::string SpecPath = BaseDir + "/" + Dep + ".ads";
        std::string ModulePath = serialization::getModuleFilePath(SpecPath);

        // Check if module file exists and is up-to-date
        if (serialization::moduleFileIsCurrent(ModulePath, SpecPath)) {
          std::cerr << "ac1: loading module file: " << ModulePath << "\n";
          auto Module = serialization::readModuleFile(ModulePath);
          if (Module) {
            LoadedModules.push_back(std::move(Module));
          }
        }
      }

      // Parse
      AdaParser Parser(Opts.InputFile, Source);
      auto Unit = Parser.ParseLibraryUnit();
      if (!Unit) {
        std::cerr << "ac1: error: parse failed\n";
        return 1;
      }

      // Semantic analysis
      SemanticAnalyzer Analyzer;
      bool SemResult = Analyzer.AnalyzeLibraryUnit(*Unit);
      if (!SemResult || GetDiagnostics().HasErrors()) {
        std::cerr << "ac1: error: semantic analysis failed\n";
        return 1;
      }

      // Code generation
      llvm::LLVMContext LLVMCtx;
      AdaCodeGen CG(LLVMCtx, Opts.InputFile, Opts.EnableContracts, Opts.OptimizationLevel);
      bool CGResult = CG.Generate(*Unit, Analyzer);
      if (!CGResult) {
        std::cerr << "ac1: error: code generation failed\n";
        return 1;
      }

      // Print IR to stdout (consumed by FileCheck tests).
      CG.GetModule()->print(llvm::outs(), nullptr);

      // Write the IR to the output file so the driver's downstream
      // "cc1 -x ir" step can turn it into a real object file.
      if (!Opts.OutputFile.empty()) {
        std::string IR;
        llvm::raw_string_ostream SS(IR);
        CG.GetModule()->print(SS, nullptr);
        SS.flush();
        if (!WriteFile(Opts.OutputFile, IR)) {
          std::cerr << "ac1: error: could not write output file '"
                    << Opts.OutputFile << "'\n";
          return 1;
        }
      }

      // Emit module file for spec compilation
      // Use OriginalSourceFile (if set) to detect spec files, otherwise fall back to InputFile
      {
        std::string SourceFile = Opts.OriginalSourceFile.empty() ? Opts.InputFile : Opts.OriginalSourceFile;
        std::string Ext = SourceFile.substr(SourceFile.rfind('.'));
        if (Ext == ".ads" || Ext == ".Ads" || Ext == ".ADS") {
          std::string ModulePath = serialization::getModuleFilePath(SourceFile);
          serialization::writeModuleFile(ModulePath, Unit->Name, Source, 0, Unit.get());
          std::cerr << "ac1: wrote module file: " << ModulePath << "\n";
        }
      }

      return 0;
    }

    case AdaActionKind::EmitPreprocessed: {
      // Write source to output file (no actual preprocessing yet)
      // If -M is also passed, perform dependency scan first.
      if (Opts.EmitDepfile || !Opts.Depfile.empty()) {
        auto Deps = ExtractWithClauseDeps(Source);

        // Determine target name from input file
        std::string Target = Opts.DepTarget;
        if (Target.empty()) {
          std::string Base = Opts.InputFile;
          size_t Slash = Base.rfind('/');
          if (Slash != std::string::npos) {
            Base = Base.substr(Slash + 1);
          }
          size_t Dot = Base.rfind('.');
          if (Dot != std::string::npos) {
            Target = Base.substr(0, Dot) + ".o";
          } else {
            Target = Base + ".o";
          }
        }

        if (EmitDependencyFile(Target, Deps, Opts.Depfile) != 0) {
          std::cerr << "ac1: error: could not emit dependency file\n";
          return 1;
        }
      }

      if (Opts.OutputFile.empty()) {
        std::cerr << "ac1: error: -o required for -emit-ada-preprocessed\n";
        return 1;
      }
      if (!WriteFile(Opts.OutputFile, Source)) {
        std::cerr << "ac1: error: could not write output file '"
                  << Opts.OutputFile << "'\n";
        return 1;
      }
      return 0;
    }
  }

  return 1;
}

} // namespace ada
} // namespace clang