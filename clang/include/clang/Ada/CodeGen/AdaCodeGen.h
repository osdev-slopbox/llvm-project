//===--- AdaCodeGen.h - Ada to LLVM IR code generator --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_ADA_CODEGEN_ADACODEGEN_H
#define CLANG_ADA_CODEGEN_ADACODEGEN_H

#include "clang/Ada/Parser/AdaParser.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace clang {
namespace ada {

class SemanticAnalyzer;

/// AdaCodeGen - Generates LLVM IR from an Ada AST.
///
/// Generation is two-pass:
///   Pass 1: collect all type and subprogram declarations, pre-declare
///           every subprogram as an llvm::Function (so calls resolve).
///   Pass 2: generate bodies for subprograms that have them.
///
/// Named type references (e.g. `P : Point;`) are resolved against the
/// TypeNameMap registered during generation; the parser represents them as
/// ScalarType nodes carrying the type name.
class AdaCodeGen {
public:
  AdaCodeGen(llvm::LLVMContext &Context, const std::string &ModuleName,
             bool EnableContracts = true, int OptimizationLevel = 0);
  ~AdaCodeGen() = default;

  /// Generate code for a library unit.
  bool Generate(LibraryUnit &Unit, SemanticAnalyzer &Analyzer);

  /// Emit the main-registration hook: procedure library units append a
  /// constructor to llvm.global_ctors that registers the unit procedure
  /// with the runtime (__ada_main_register); the runtime's main() runs
  /// the first registered procedure.
  void EmitMainRegistration(DeclarationNode *Root);

  /// Get the generated LLVM module.
  llvm::Module *GetModule() { return Module.get(); }

private:
  llvm::LLVMContext &Context;
  std::unique_ptr<llvm::Module> Module;
  llvm::IRBuilder<> Builder;

  /// Map from Ada types to LLVM types (per AST node pointer).
  std::map<TypeNode *, llvm::Type *> TypeMap;

  /// Map from identifier names to LLVM values (current scope).
  std::map<std::string, llvm::Value *> NameMap;

  /// Map from lowercase type name to the defining TypeNode.
  std::map<std::string, TypeNode *> TypeNameMap;

  /// Map from variable name to its declared TypeNode (for field access).
  std::map<std::string, TypeNode *> VarTypes;

  /// Map from lowercase subprogram name to the pre-declared function.
  std::map<std::string, llvm::Function *> FunctionMap;

  /// All subprograms collected in declaration order.
  std::vector<SubprogramDeclaration *> Subprograms;

  /// Stack of loop-exit blocks (innermost last); `exit` branches to the
  /// top. Pushed by loop codegen.
  std::vector<llvm::BasicBlock *> LoopExits;

  /// Tagged-type registries (RM 3.9.2); keys are lowercase type names.
  /// TaggedLayouts: full field list (all ancestor fields + own part, in
  /// declaration order) as (name, LLVM type) pairs.
  std::map<std::string, std::vector<std::pair<std::string, llvm::Type *>>>
      TaggedLayouts;

  /// One i8 tag global per tagged type (__ada_tag_<Name>, values 1..N in
  /// declaration order); object layouts store the global's *address*.
  std::map<std::string, llvm::GlobalVariable *> TagGlobals;

  /// Parent type name per tagged type ("" for the root).
  std::map<std::string, std::string> TaggedParent;

  /// Named (acyclic) LLVM struct per tagged type: {ptr tag, fields...}.
  std::map<std::string, llvm::StructType *> TaggedStructMap;

  /// Tagged type names in tag-value (declaration) order.
  std::vector<std::string> TaggedOrder;

  /// Subprogram declaration pointer to function (call disambiguation).
  std::map<SubprogramDeclaration *, llvm::Function *> DeclToFunction;

  /// Mangled function name per subprogram declaration.
  std::map<SubprogramDeclaration *, std::string> MangledNames;

  /// Next tag value to assign (1-based).
  unsigned NextTagValue = 1;

  /// Whether Pre/Post contract assertions are emitted (-fno-ada-contracts).
  bool ContractsEnabled;

  /// Optimization level (0, 1, 2, or 3).
  int OptimizationLevel;

  /// Alloca holding the current function's return value so Post aspects can
  /// read 'Result. Null for void subprograms.
  llvm::Value *CurrentResult;

  /// The subprogram declaration currently being generated.
  SubprogramDeclaration *CurrentDecl;

  /// Objects that need Finalize called at scope exit (RM 3.9.4).
  /// Each entry is (variable name, type name).
  std::vector<std::pair<std::string, std::string>> FinalizeList;

  /// Emit a Constraint_Error raise for a failed contract assertion.
  void EmitContractViolation();

  /// Generate LLVM type for an Ada type.
  llvm::Type *GenerateType(TypeNode *Type);

  /// Resolve a (possibly named-reference) type to its RecordType definition.
  RecordType *ResolveRecordType(const TypeNode *Type);

  /// Collect types and subprograms from a declaration tree (pass 1).
  void CollectDeclarations(DeclarationNode *Decl);

  /// Create the llvm::Function for a subprogram without a body (pass 1).
  llvm::Function *PredeclareFunction(SubprogramDeclaration *Decl);

  /// Register a tagged type (idempotent): tag global + full field layout
  /// (root fields from the record type, derived = parent layout + own part).
  void RegisterTaggedType(TypeDeclaration *TD);

  /// Named LLVM struct for a tagged type: {ptr tag, fields...}.
  llvm::StructType *TaggedStructType(const std::string &Name);

  /// Mangled name for a subprogram (overload disambiguation by the first
  /// formal's type name, then a counter); collision-checked.
  std::string MangledName(SubprogramDeclaration *Decl);

  /// Lowercase type name of the first formal parameter ("" if none).
  static std::string FirstFormalTypeName(SubprogramDeclaration *Decl);

  /// T plus its proper tagged descendants in tag-value order.
  std::vector<std::string> TaggedDescendants(const std::string &T) const;

  /// List of task names (for pthread creation)
  std::vector<std::string> TaskNames;

  /// Current task being generated (for accept statement codegen)
  TaskDeclaration *CurrentTask = nullptr;

  /// Tagged assignment (tag pointer + destination fields copied by name
  /// from the source layout); false when not applicable.
  bool GenerateTaggedAssignment(llvm::Value *Target, llvm::Value *Source,
                                const std::string &TargetType,
                                const std::string &SourceType);

  /// Dynamic dispatch on a tagged first argument (RM 3.9.4); false when the
  /// call is not a dispatchable tagged call.
  bool GenerateTaggedDispatch(const CallExpression *Call,
                              llvm::Function *StaticCallee,
                              SubprogramDeclaration *StaticDecl,
                              std::vector<llvm::Value *> &Args,
                              llvm::Value *&Result);

  /// Generate the body of a subprogram into its pre-declared function.
  void GenerateBody(SubprogramDeclaration *Decl, llvm::Function *Func);

  /// Generate code for a generic instantiation (instantiation expansion).
  void GenerateGenericInstantiation(GenericInstantiation *Inst,
                                    SemanticAnalyzer &Analyzer);

  /// Generate code for a declaration.
  bool GenerateDeclaration(DeclarationNode *Decl);

  /// Generate code for a task body (pthread wrapper).
  void GenerateTaskBody(TaskBody *Body, SemanticAnalyzer &Analyzer);

  /// Generate code for task activation (pthread creation).
  void GenerateTask(TaskDeclaration *Task, SemanticAnalyzer &Analyzer);

  /// Generate code for a list of statements.
  void GenerateStatements(
      const std::vector<std::unique_ptr<StatementNode>> &Stmts);

  /// Generate code for a statement.
  void GenerateStatement(StatementNode *Stmt);

  /// Generate an l-value: a pointer to the storage of Expr.
  /// Sets ElemType to the pointed-to LLVM type. Returns null on failure.
  llvm::Value *GenerateLValue(const ExpressionNode *Expr, llvm::Type *&ElemType);

  /// Emit a branch to Target unless the current block already ends with a
  /// terminator (e.g. a `return` in an if-branch).
  void BranchUnlessTerminated(llvm::BasicBlock *Target);

  /// Emit the longjmp-based try mechanism around Body with the given
  /// exception handlers (P3 skeleton; see ada_runtime.c). At entry the
  /// current insert block is the caller's block; on return the insert
  /// point is the merge block.
  void GenerateTryMechanism(
      const std::vector<std::unique_ptr<StatementNode>> &Body,
      const ExceptionHandlerList &Handlers);

  /// Generate the arguments and call for a call expression.
  llvm::Value *GenerateCall(CallExpression *Call);

  /// Lower a call to a runtime-library builtin (Text_IO, Integer_Text_IO,
  /// Float_Text_IO, Calendar, Strings, Environment_Variables). Returns true
  /// when FnName was recognized and handled; Result receives the call value
  /// for value-returning builtins (null for procedures).
  bool GenerateRuntimeCall(
      const std::string &FnName,
      const std::vector<std::unique_ptr<ExpressionNode>> &Args,
      llvm::Value *&Result);

  /// Generate code for an expression.
  llvm::Value *GenerateExpression(ExpressionNode *Expr);

  static std::string ToLower(std::string S);
};

} // namespace ada
} // namespace clang

#endif // CLANG_ADA_CODEGEN_ADACODEGEN_H
