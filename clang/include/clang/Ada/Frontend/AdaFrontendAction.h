//===--- AdaFrontendAction.h - Ada 2012 frontend action ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ac1 frontend actions: compile, syntax-only, dependency-scan, ast-dump.
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_ADA_FRONTEND_ADAFRONTENDACTION_H
#define CLANG_ADA_FRONTEND_ADAFRONTENDACTION_H

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace clang {
namespace ada {

// Frontend action kind
enum class AdaActionKind {
  Compile,
  SyntaxOnly,
  DependencyScan,
  ASTDump,
  EmitPreprocessed
};

// ac1 command-line argument handling
struct AdaFrontendOptions {
  std::string InputFile;
  std::string OutputFile;
  std::string OriginalSourceFile;
  AdaActionKind Action = AdaActionKind::Compile;
  bool SyntaxOnly = false;
  bool EmitDepfile = false;
  std::string Depfile;
  std::string DepTarget;
  bool Quiet = false;
  bool EnableContracts = true;
  int OptimizationLevel = 0; // 0, 1, 2, or 3
};

// Diagnostic severity levels
enum class DiagSeverity {
  Note,
  Warning,
  Error
};

// Warning categories. Each warning belongs to one of these groups; the
// -Wall/-Wextra/-Wpedantic flags enable groups, -Wno-ada-<name> disables
// individual warnings, and -Werror[=name] promotes them.
enum class WarningKind {
  UnknownPragma,      // -Wunknown-pragmas (in -Wall)
  UnknownAspect,      // -Wunknown-aspects (in -Wall)
  UnusedVariable,     // -Wunused-variable (in -Wall)
  UnusedParameter,    // -Wunused-parameter (in -Wextra)
  UnreachableCode,    // -Wunreachable-code (in -Wextra)
  UnusedWith,         // -Wunused-with (in -Wextra)
  ComparisonAlways,   // -Wtype-limits (in -Wextra)
  Shadowing,          // -Wshadow (in -Wextra)
  Pedantic,           // -Wpedantic
  Deprecated,         // -Wdeprecated (in -Wall)
  InvalidPragmaValue, // -Winvalid-pragma-value (in -Wall)
};

// Warning level requested by the user.
enum class WarningLevel {
  None,      // no warnings
  Default,   // default warnings (unused pragma etc.)
  All,       // -Wall
  Extra,     // -Wextra
  Pedantic   // -Wpedantic
};

// Diagnostic manager with warning levels and categories.
class Diagnostics {
public:
  Diagnostics()
      : ErrorCount(0), WarningCount(0), Werror(false), WerrorAll(false),
        Level(WarningLevel::Default) {
    // Default-on warnings.
    EnabledWarnings.insert(WarningKind::UnknownPragma);
    EnabledWarnings.insert(WarningKind::UnknownAspect);
    EnabledWarnings.insert(WarningKind::UnusedVariable);
  }

  void SetWerror(bool Value) { Werror = Value; }
  void SetWerrorAll(bool Value) { WerrorAll = Value; }

  /// Enable a warning level; groups are cumulative (-Wall + -Wextra).
  void SetWarningLevel(WarningLevel L) {
    if (L == WarningLevel::None) {
      EnabledWarnings.clear();
      Level = L;
      return;
    }
    if (static_cast<int>(L) > static_cast<int>(Level))
      Level = L;
    if (L == WarningLevel::All || L == WarningLevel::Extra ||
        L == WarningLevel::Pedantic) {
      EnabledWarnings.insert(WarningKind::Deprecated);
      EnabledWarnings.insert(WarningKind::InvalidPragmaValue);
    }
    if (L == WarningLevel::Extra || L == WarningLevel::Pedantic) {
      EnabledWarnings.insert(WarningKind::UnusedParameter);
      EnabledWarnings.insert(WarningKind::UnreachableCode);
      EnabledWarnings.insert(WarningKind::UnusedWith);
      EnabledWarnings.insert(WarningKind::ComparisonAlways);
      EnabledWarnings.insert(WarningKind::Shadowing);
    }
    if (L == WarningLevel::Pedantic)
      EnabledWarnings.insert(WarningKind::Pedantic);
  }

  /// Disable a specific warning (-Wno-ada-<name>).
  void DisableWarning(WarningKind Kind) { EnabledWarnings.erase(Kind); }

  /// Enable a specific warning (-Wada-<name>).
  void EnableWarning(WarningKind Kind) { EnabledWarnings.insert(Kind); }

  bool IsWarningEnabled(WarningKind Kind) const {
    return EnabledWarnings.count(Kind) != 0;
  }

  /// Emit a diagnostic. `Kind` identifies the warning category (ignored for
  /// notes and errors).
  void Diag(const std::string &Loc, DiagSeverity Severity,
            const std::string &Msg, WarningKind Kind = WarningKind::Pedantic) {
    if (Severity == DiagSeverity::Warning) {
      if (!IsWarningEnabled(Kind))
        return;
      WarningCount++;
      Print(Loc, "warning: " + Msg, WarningOptionName(Kind));
      if (Werror || WerrorAll)
        ErrorCount++;
      return;
    }
    if (Severity == DiagSeverity::Error) {
      ErrorCount++;
      Print(Loc, "error: " + Msg, "");
      return;
    }
    Print(Loc, "note: " + Msg, "");
  }

  /// Convenience: emit a note attached to a previous diagnostic.
  void Note(const std::string &Loc, const std::string &Msg) {
    Print(Loc, "note: " + Msg, "");
  }

  /// Emit a warning that is not subject to group filtering (used for
  /// command-line problems like unknown -W options).
  void WarnAlways(const std::string &Msg) {
    WarningCount++;
    Print("", "warning: " + Msg, "");
  }

  bool HasErrors() const { return ErrorCount > 0; }
  int GetWarningCount() const { return WarningCount; }

private:
  void Print(const std::string &Loc, const std::string &Text,
             const std::string &Opt) {
    if (!Loc.empty())
      std::cerr << Loc << ": ";
    std::cerr << Text;
    if (!Opt.empty())
      std::cerr << " [" << Opt << "]";
    std::cerr << "\n";
  }

  static std::string WarningOptionName(WarningKind Kind) {
    switch (Kind) {
    case WarningKind::UnknownPragma:      return "-Wunknown-pragmas";
    case WarningKind::UnknownAspect:      return "-Wunknown-aspects";
    case WarningKind::UnusedVariable:     return "-Wunused-variable";
    case WarningKind::UnusedParameter:    return "-Wunused-parameter";
    case WarningKind::UnreachableCode:    return "-Wunreachable-code";
    case WarningKind::UnusedWith:         return "-Wunused-with";
    case WarningKind::ComparisonAlways:   return "-Wtype-limits";
    case WarningKind::Shadowing:          return "-Wshadow";
    case WarningKind::Pedantic:           return "-Wpedantic";
    case WarningKind::Deprecated:         return "-Wdeprecated";
    case WarningKind::InvalidPragmaValue: return "-Winvalid-pragma-value";
    }
    return "";
  }

  int ErrorCount;
  int WarningCount;
  bool Werror;
  bool WerrorAll;
  WarningLevel Level;
  std::set<WarningKind> EnabledWarnings;
};

// Global diagnostics instance
Diagnostics &GetDiagnostics();

// Parse ac1 arguments
bool ParseAc1Args(int Argc, const char * const *Argv, AdaFrontendOptions &Opts);

// Run the appropriate frontend action
int RunFrontendAction(const AdaFrontendOptions &Opts);

} // namespace ada
} // namespace clang

#endif // CLANG_ADA_FRONTEND_ADAFRONTENDACTION_H