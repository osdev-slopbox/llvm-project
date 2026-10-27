//===--- AdaCodeGen.cpp - Ada to LLVM IR code generator ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/CodeGen/AdaCodeGen.h"
#include "clang/Ada/Semantics/SemanticAnalyzer.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"

#include <cctype>
#include <iostream>

using namespace clang;
using namespace clang::ada;

AdaCodeGen::AdaCodeGen(llvm::LLVMContext &Context,
                       const std::string &ModuleName, bool EnableContracts,
                       int OptimizationLevel)
    : Context(Context),
      Module(std::make_unique<llvm::Module>(ModuleName, Context)),
      Builder(Context),
      ContractsEnabled(EnableContracts),
      OptimizationLevel(OptimizationLevel),
      CurrentResult(nullptr),
      CurrentDecl(nullptr) {
  // Set module optimization level attribute for the backend
  Module->addModuleFlag(llvm::Module::Override, "opt-level",
                        llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context),
                                               OptimizationLevel));
}

std::string AdaCodeGen::ToLower(std::string S) {
  for (char &C : S)
    C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
  return S;
}

bool AdaCodeGen::Generate(LibraryUnit &Unit, SemanticAnalyzer &Analyzer) {
  if (!Unit.Root) {
    return true;
  }

  auto *Root = static_cast<DeclarationNode *>(Unit.Root.get());

  // Pass 1: collect subprograms and register package-level types.
  CollectDeclarations(Root);

  // Pre-declare all subprograms so calls between them can resolve.
  for (SubprogramDeclaration *Sub : Subprograms) {
    PredeclareFunction(Sub);
  }

  // Pass 2: generate bodies.
  for (SubprogramDeclaration *Sub : Subprograms) {
    if (!Sub->BodyStatements.empty()) {
      GenerateBody(Sub, DeclToFunction[Sub]);
    }
  }

  // Generate task activations
  // Note: TaskNames only contains task declarations collected in CollectDeclarations
  // For immediate bodies, we need to process them directly from the AST
  if (Root->getKind() == NodeKind::TaskDeclaration) {
    auto *Task = static_cast<TaskDeclaration *>(Root);
    GenerateTask(Task, Analyzer);
  } else if (Root->getKind() == NodeKind::SubprogramDeclaration) {
    // Check for tasks declared within the subprogram
    auto *Sub = static_cast<SubprogramDeclaration *>(Root);
    for (auto &D : Sub->Declarations) {
      if (D && D->getKind() == NodeKind::TaskDeclaration) {
        auto *Task = static_cast<TaskDeclaration *>(D.get());
        GenerateTask(Task, Analyzer);
      }
    }
  }

  // Handle generic instantiations: expand each by creating a copy of
  // the generic body with substituted types (instantiation expansion).
  if (Unit.Root->getKind() == NodeKind::GenericInstantiation) {
    GenerateGenericInstantiation(
        static_cast<GenericInstantiation *>(Unit.Root.get()), Analyzer);
  }

  // Register the main entry point with the runtime.
  EmitMainRegistration(Root);

  return true;
}

void AdaCodeGen::GenerateGenericInstantiation(
    GenericInstantiation *Inst, SemanticAnalyzer &Analyzer) {
  // For P4, instantiate by creating a function that calls the generic body.
  // The instantiated function has the same signature as the generic.
  // Type substitution (different actual types) is a future enhancement.
  
  // Find the generic subprogram in the function map
  std::string GenericKey = ToLower(Inst->GenericName);
  auto It = FunctionMap.find(GenericKey);
  if (It == FunctionMap.end()) {
    // Generic not found in this compilation unit; it's in a separate
    // library item. Create a declaration for the instantiated subprogram.
    // The body will be linked from the generic's object file.
    return;
  }

  // Generic found; create an instantiated copy that calls the generic.
  auto *GenericFunc = It->second;
  std::string InstKey = ToLower(Inst->Name);
  
  // Create a new function with the instantiated name
  auto *GenericFT = llvm::dyn_cast<llvm::FunctionType>(GenericFunc->getFunctionType());
  llvm::Function *InstFunc = llvm::Function::Create(
      GenericFT,
      GenericFunc->getLinkage(),
      InstKey,
      Module.get());
  
  // Map the instantiated name to the new function
  FunctionMap[InstKey] = InstFunc;
  
  // Generate the body: forward all arguments to the generic function
  auto *Entry = llvm::BasicBlock::Create(Context, "entry", InstFunc);
  Builder.SetInsertPoint(Entry);
  
  // Collect arguments
  std::vector<llvm::Value *> Args;
  for (auto &Arg : InstFunc->args()) {
    Args.push_back(&Arg);
  }
  
  // Call the generic function
  llvm::CallInst *Call = Builder.CreateCall(GenericFunc, Args, "call");
  
  // Return the result
  if (GenericFT->isVoidTy()) {
    Builder.CreateRetVoid();
  } else {
    Builder.CreateRet(Call);
  }
}

void AdaCodeGen::EmitMainRegistration(DeclarationNode *Root) {
  // Only procedure library units register a main entry point.
  if (Root->getKind() != NodeKind::SubprogramDeclaration) {
    return;
  }
  auto *Sub = static_cast<SubprogramDeclaration *>(Root);
  if (Sub->IsFunction || Sub->BodyStatements.empty()) {
    return;
  }
  auto DIt = DeclToFunction.find(Sub);
  if (DIt == DeclToFunction.end()) {
    return;
  }
  llvm::Function *Func = DIt->second;

  // A small internal constructor that registers the unit procedure.
  auto *Ctor = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {}, false),
      llvm::Function::InternalLinkage, "__ada_ctor_" + Sub->Name, *Module);
  auto *BB = llvm::BasicBlock::Create(Context, "entry", Ctor);
  Builder.SetInsertPoint(BB);
  auto *PtrTy = llvm::PointerType::get(Context, 0);
  auto *RegTy =
      llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {PtrTy}, false);
  auto Reg = Module->getOrInsertFunction("__ada_main_register", RegTy);
  Builder.CreateCall(Reg, {llvm::ConstantExpr::getBitCast(Func, PtrTy)});
  Builder.CreateRetVoid();

  // Append to llvm.global_ctors (merged by the linker across objects;
  // object link order == source order, so the first registered procedure
  // is the main unit).
  auto *I32 = llvm::Type::getInt32Ty(Context);
  auto *EntryTy = llvm::StructType::get(I32, PtrTy, PtrTy);
  auto *Entry = llvm::ConstantStruct::get(EntryTy,
                                          {llvm::ConstantInt::get(I32, 65535),
                                           llvm::ConstantExpr::getBitCast(Ctor, PtrTy),
                                           llvm::ConstantPointerNull::get(PtrTy)});

  llvm::GlobalVariable *GV = Module->getGlobalVariable("llvm.global_ctors");
  if (GV) {
    // Defensive: merge with an existing registration (one library unit per
    // module in v1, so this is not expected).
    auto *GVInit = GV->getInitializer();
    size_t N = GVInit ? GVInit->getNumOperands() : 0;
    auto *NewArrTy = llvm::ArrayType::get(EntryTy, N + 1);
    std::vector<llvm::Constant *> Entries;
    if (GVInit)
      for (size_t I = 0; I < N; ++I)
        Entries.push_back(llvm::cast<llvm::Constant>(GVInit->getOperand(I)));
    Entries.push_back(Entry);
    GV->eraseFromParent();
    GV = new llvm::GlobalVariable(*Module, NewArrTy, false,
                                  llvm::GlobalValue::AppendingLinkage,
                                  llvm::ConstantArray::get(NewArrTy, Entries),
                                  "llvm.global_ctors");
  } else {
    auto *ArrTy = llvm::ArrayType::get(EntryTy, 1);
    GV = new llvm::GlobalVariable(*Module, ArrTy, false,
                                  llvm::GlobalValue::AppendingLinkage,
                                  llvm::ConstantArray::get(ArrTy, {Entry}),
                                  "llvm.global_ctors");
  }
}

void AdaCodeGen::CollectDeclarations(DeclarationNode *Decl) {
  if (!Decl) {
    return;
  }

  switch (Decl->getKind()) {
  case NodeKind::PackageDeclaration: {
    auto *Pkg = static_cast<PackageDeclaration *>(Decl);
    for (auto &D : Pkg->Declarations) {
      CollectDeclarations(D.get());
    }
    break;
  }
  case NodeKind::SubprogramDeclaration: {
    auto *Sub = static_cast<SubprogramDeclaration *>(Decl);
    Subprograms.push_back(Sub);
    // Register subprogram-local types now, not when the body is generated:
    // PredeclareFunction resolves parameter types via TypeNameMap, and an
    // unknown tagged name would silently fall back to i32 (a ptr-into-i32
    // formal fails the verifier). Nested subprograms are pre-declared too.
    for (auto &D : Sub->Declarations) {
      if (!D) {
        continue;
      }
      if (D->getKind() == NodeKind::SubprogramDeclaration) {
        CollectDeclarations(D.get());
      } else if (D->getKind() == NodeKind::TypeDeclaration) {
        auto *TD = static_cast<TypeDeclaration *>(D.get());
        if (TD->Type) {
          TypeNameMap[ToLower(TD->Name)] = TD->Type.get();
        }
        if (TD->IsTagged) {
          RegisterTaggedType(TD);
        }
      }
    }
    break;
  }
  case NodeKind::TypeDeclaration: {
    auto *TD = static_cast<TypeDeclaration *>(Decl);
    if (TD->Type) {
      TypeNameMap[ToLower(TD->Name)] = TD->Type.get();
    }
    if (TD->IsTagged) {
      RegisterTaggedType(TD);
    }
    break;
  }
  case NodeKind::TaskDeclaration: {
    auto *Task = static_cast<TaskDeclaration *>(Decl);
    TaskNames.push_back(Task->Name);
    break;
  }
  default:
    break;
  }
}

llvm::Function *AdaCodeGen::PredeclareFunction(SubprogramDeclaration *Decl) {
  std::string Mangled = MangledName(Decl);
  std::string Key = ToLower(Mangled);
  auto It = FunctionMap.find(Key);
  if (It != FunctionMap.end()) {
    return It->second;
  }

  std::vector<llvm::Type *> ParamTypes;
  for (auto &Param : Decl->FormalParameters) {
    if (Param) {
      // Fall back to i32 when the parameter type is missing (parse error
      // recovery) so that FunctionType::get never sees void/null.
      llvm::Type *PTy = Param->Type ? GenerateType(Param->Type.get())
                                    : llvm::Type::getInt32Ty(Context);
      // Aggregates (records, tagged) are passed by reference: the caller
      // passes the storage pointer (see the GenerateCall heuristic).
      if (PTy->isStructTy()) {
        PTy = llvm::PointerType::getUnqual(Context);
      }
      ParamTypes.push_back(PTy);
    }
  }

  llvm::Type *RetTy = llvm::Type::getVoidTy(Context);
  if (Decl->IsFunction) {
    // Ada functions always return a value; fall back to i32 when the
    // return type is missing (parse error recovery).
    RetTy = Decl->ReturnType ? GenerateType(Decl->ReturnType.get())
                             : llvm::Type::getInt32Ty(Context);
  }

  auto *FTy = llvm::FunctionType::get(RetTy, ParamTypes, false);
  auto *Func = llvm::Function::Create(FTy, llvm::Function::ExternalLinkage,
                                      Mangled, *Module);

  // Set parameter names.
  size_t I = 0;
  for (auto &Arg : Func->args()) {
    if (I < Decl->FormalParameters.size() && Decl->FormalParameters[I]) {
      Arg.setName(Decl->FormalParameters[I]->Name);
    }
    I++;
  }

  MangledNames[Decl] = Mangled;
  FunctionMap[Key] = Func;
  DeclToFunction[Decl] = Func;
  return Func;
}

std::string AdaCodeGen::FirstFormalTypeName(SubprogramDeclaration *Decl) {
  for (auto &Param : Decl->FormalParameters) {
    if (Param && Param->Type) {
      if (Param->Type->getKind() == NodeKind::ScalarType) {
        return static_cast<ScalarType *>(Param->Type.get())->Name;
      }
      return "t";
    }
  }
  return "";
}

std::string AdaCodeGen::MangledName(SubprogramDeclaration *Decl) {
  auto It = MangledNames.find(Decl);
  if (It != MangledNames.end()) {
    return It->second;
  }

  std::string Name = Decl->Name;
  auto Taken = [&](const std::string &Candidate) {
    for (auto &KV : MangledNames) {
      if (KV.second == Candidate) {
        return true;
      }
    }
    return false;
  };

  if (Taken(Name)) {
    // Another subprogram already claims the bare name: disambiguate with
    // the lowercased first-formal type, then a numeric suffix.
    std::string Base = Name + "_" + ToLower(FirstFormalTypeName(Decl));
    if (!Taken(Base)) {
      Name = Base;
    } else {
      unsigned N = 1;
      std::string Candidate;
      do {
        Candidate = Name + "_" + std::to_string(N);
        N++;
      } while (Taken(Candidate));
      Name = Candidate;
    }
  }

  MangledNames[Decl] = Name;
  return Name;
}

std::vector<std::string>
AdaCodeGen::TaggedDescendants(const std::string &T) const {
  std::vector<std::string> Result;
  Result.push_back(T);
  // Walk up each type's parent chain to find every descendant, in tag order.
  for (const auto &Name : TaggedOrder) {
    if (Name == T) {
      continue;
    }
    std::string Walk = Name;
    unsigned Depth = 0;
    while (Walk != T && Depth < 64) {
      auto PIt = TaggedParent.find(Walk);
      if (PIt == TaggedParent.end()) {
        break;
      }
      Walk = PIt->second;
      Depth++;
    }
    if (Walk == T) {
      Result.push_back(Name);
    }
  }
  return Result;
}

void AdaCodeGen::RegisterTaggedType(TypeDeclaration *TD) {
  if (!TD || !TD->IsTagged) {
    return;
  }
  std::string Name = TD->Name;
  std::string Key = ToLower(Name);
  if (TaggedParent.count(Key)) {
    return; // already registered
  }

  // Tag global: one i8 per tagged type, values 1..N in declaration order.
  auto *TagTy = llvm::IntegerType::getInt8Ty(Context);
  auto *Tag = new llvm::GlobalVariable(*Module, TagTy, /*isConstant=*/true,
                                       llvm::GlobalValue::InternalLinkage,
                                       llvm::ConstantInt::get(TagTy, NextTagValue++),
                                       "__ada_tag_" + Name);
  TagGlobals[Key] = Tag;
  TaggedOrder.push_back(Key);

  // Full layout: parent layout + own fields.
  std::vector<std::pair<std::string, llvm::Type *>> Fields;
  if (!TD->Parent.empty()) {
    std::string PKey = ToLower(TD->Parent);
    auto LIt = TaggedLayouts.find(PKey);
    if (LIt != TaggedLayouts.end()) {
      Fields = LIt->second;
    }
    TaggedParent[Key] = PKey;
  } else {
    TaggedParent[Key] = "";
  }

  TypeNode *T = TD->Type.get();
  // T == nullptr is a root tagged null record: no fields, just the tag slot.
  if (T && T->getKind() == NodeKind::DerivedType) {
    auto *DT = static_cast<DerivedType *>(T);
    for (auto &Ext : DT->Extensions) {
      if (!Ext) {
        continue;
      }
      // Extensions are object declarations; take name and type.
      if (Ext->getKind() == NodeKind::ObjectDeclaration) {
        auto *Obj = static_cast<ObjectDeclaration *>(Ext.get());
        llvm::Type *FTy = Obj->Type ? GenerateType(Obj->Type.get())
                                    : llvm::Type::getInt32Ty(Context);
        Fields.emplace_back(Obj->Name, FTy);
      }
    }
  } else if (T && T->getKind() == NodeKind::RecordType) {
    auto *RT = static_cast<RecordType *>(T);
    for (auto &F : RT->Fields) {
      if (!F) {
        continue;
      }
      llvm::Type *FTy = F->Type ? GenerateType(F->Type.get())
                                : llvm::Type::getInt32Ty(Context);
      Fields.emplace_back(F->Name, FTy);
    }
  }

  TaggedLayouts[Key] = Fields;
}

llvm::StructType *AdaCodeGen::TaggedStructType(const std::string &Name) {
  std::string Key = ToLower(Name);
  auto It = TaggedStructMap.find(Key);
  if (It != TaggedStructMap.end()) {
    return It->second;
  }
  auto LIt = TaggedLayouts.find(Key);
  if (LIt == TaggedLayouts.end()) {
    return llvm::StructType::create(Context, "__ada_tagged_" + Key);
  }
  std::vector<llvm::Type *> Fields;
  Fields.push_back(llvm::PointerType::getUnqual(Context)); // tag slot
  for (auto &F : LIt->second) {
    Fields.push_back(F.second);
  }
  auto *STy = llvm::StructType::create(Context, Fields, "__ada_tagged_" + Key);
  TaggedStructMap[Key] = STy;
  return STy;
}

void AdaCodeGen::GenerateBody(SubprogramDeclaration *Decl,
                              llvm::Function *Func) {
  // Scope guards: save and restore name/type maps around the body so
  // locals of one subprogram do not leak into the next.
  auto NameMapSave = NameMap;
  auto TypeNameMapSave = TypeNameMap;
  auto VarTypesSave = VarTypes;
  auto FinalizeListSave = FinalizeList;

  // Register types declared locally in this subprogram. TypeNameMap is
  // saved/restored per body, so re-register here; the module-level tagged
  // registries are idempotent and already populated by CollectDeclarations.
  for (auto &D : Decl->Declarations) {
    if (D && D->getKind() == NodeKind::TypeDeclaration) {
      auto *TD = static_cast<TypeDeclaration *>(D.get());
      if (TD->Type) {
        TypeNameMap[ToLower(TD->Name)] = TD->Type.get();
      }
      RegisterTaggedType(TD);
    }
  }

  auto *Entry = llvm::BasicBlock::Create(Context, "entry", Func);
  Builder.SetInsertPoint(Entry);

  // Track the subprogram under generation and allocate the 'Result slot
  // that Post aspects read (RM 5.5.1(16)).
  CurrentDecl = Decl;
  CurrentResult = nullptr;
  if (!Func->getReturnType()->isVoidTy()) {
    CurrentResult = Builder.CreateAlloca(Func->getReturnType(), nullptr, "result");
  }

  // Bind formal parameters. Record the parameter type in VarTypes so that
  // field-access and assignment lowering can see aggregate (tagged) params.
  size_t I = 0;
  for (auto &Arg : Func->args()) {
    if (I < Decl->FormalParameters.size() && Decl->FormalParameters[I]) {
      NameMap[Decl->FormalParameters[I]->Name] = &Arg;
      VarTypes[Decl->FormalParameters[I]->Name] =
          Decl->FormalParameters[I]->Type.get();
    }
    I++;
  }

  // Precondition assertions (RM 5.5.1(8)): evaluated after parameter
  // association, before the body. A failed precondition raises
  // Constraint_Error, which is caught by the *caller's* handlers
  // (RM 5.5.1(12-15)), hence the check precedes the handler wrap.
  if (ContractsEnabled) {
    for (auto &A : Decl->Aspects) {
      if (!A || A->AspectName != "Pre" || !A->AspectValue)
        continue;
      llvm::Value *Cond = GenerateExpression(A->AspectValue.get());
      if (!Cond || !Cond->getType()->isIntegerTy(1))
        continue;
      llvm::BasicBlock *FailBB =
          llvm::BasicBlock::Create(Context, "pre.fail", Func);
      llvm::BasicBlock *OkBB = llvm::BasicBlock::Create(Context, "pre.ok", Func);
      Builder.CreateCondBr(Cond, OkBB, FailBB);
      Builder.SetInsertPoint(FailBB);
      EmitContractViolation();
      Builder.SetInsertPoint(OkBB);
    }
  }

  // Generate declarations in the declarative part.
  for (auto &D : Decl->Declarations) {
    if (D) {
      GenerateDeclaration(D.get());
    }
  }

  // Generate the body statements. Subprogram-level exception handlers
  // (RM 8.6(4)) wrap the whole body in the same longjmp mechanism as a
  // try/except block.
  if (!Decl->ExceptionHandlers.empty()) {
    GenerateTryMechanism(Decl->BodyStatements, Decl->ExceptionHandlers);
  } else {
    GenerateStatements(Decl->BodyStatements);
  }

  // Controlled types: call Finalize for objects in reverse declaration order
  // (RM 3.9.4). Insert the calls at the end of the current block, before the
  // epilogue return.
  for (auto it = FinalizeList.rbegin(); it != FinalizeList.rend(); ++it) {
    auto VarIt = NameMap.find(it->first);
    if (VarIt == NameMap.end()) {
      continue;
    }
    auto FinalIt = FunctionMap.find("finalize");
    if (FinalIt != FunctionMap.end()) {
      Builder.CreateCall(FinalIt->second, {VarIt->second});
    }
  }
  FinalizeList.clear();

  // Ensure the function ends with a return. Target the current insertion
  // block, NOT Func->back(): a trailing loop/try leaves the insertion point
  // on a merge block that is not the last block in the function (exit.next /
  // try.reraise blocks are created after it), so Func->back() would be
  // terminated while the live merge block is left empty (invalid IR).
  llvm::BasicBlock *InsertBB = Builder.GetInsertBlock();
  if (InsertBB && (InsertBB->empty() || !InsertBB->back().isTerminator())) {
    if (Decl->IsFunction) {
      Builder.CreateRet(llvm::Constant::getNullValue(Func->getReturnType()));
    } else {
      Builder.CreateRetVoid();
    }
  }

  // Restore scope state.
  NameMap = NameMapSave;
  TypeNameMap = TypeNameMapSave;
  VarTypes = VarTypesSave;
  FinalizeList = FinalizeListSave;
}

llvm::Type *AdaCodeGen::GenerateType(TypeNode *Type) {
  if (!Type) {
    return llvm::Type::getVoidTy(Context);
  }

  // Check type map first.
  auto Found = TypeMap.find(Type);
  if (Found != TypeMap.end()) {
    return Found->second;
  }

  llvm::Type *Result = nullptr;

  switch (Type->getKind()) {
  case NodeKind::ScalarType: {
    auto *Scalar = static_cast<ScalarType *>(Type);
    // String-valued objects are char pointers in the runtime.
    {
      const std::string LN = ToLower(Scalar->Name);
      if (LN == "string" || LN == "wide_string" ||
          LN == "wide_wide_string") {
        Result = llvm::PointerType::get(Context, 0);
        break;
      }
    }
    // A named type reference (e.g. `P : Point;`) is represented as a
    // ScalarType carrying the type name; resolve it against the registry.
    if (!Scalar->Name.empty()) {
      // Tagged types get a dedicated struct layout (tag slot + fields);
      // check before the generic TypeNameMap so the tagged layout wins
      // (a derived tagged type is also present there as a DerivedType,
      // which would otherwise resolve to its parent and drop the tag).
      if (TaggedLayouts.count(ToLower(Scalar->Name))) {
        return TaggedStructType(Scalar->Name);
      }
      auto It = TypeNameMap.find(ToLower(Scalar->Name));
      if (It != TypeNameMap.end()) {
        return GenerateType(It->second);
      }
    }
    switch (Scalar->ScalarKind) {
    case ScalarType::Integer:
    case ScalarType::Enumeration:
      Result = llvm::Type::getInt32Ty(Context);
      break;
    case ScalarType::Float:
      // Float is lowered to f64 so it matches the runtime's double ABI
      // (__ada_print_float, Ada.Calendar.Clock, ...).
      Result = llvm::Type::getDoubleTy(Context);
      break;
    case ScalarType::Boolean:
      Result = llvm::Type::getInt1Ty(Context);
      break;
    case ScalarType::Character:
      Result = llvm::Type::getInt32Ty(Context);
      break;
    }
    break;
  }
  case NodeKind::StringType:
    // String values are NUL-terminated char pointers in the runtime.
    Result = llvm::PointerType::get(Context, 0);
    break;
  case NodeKind::RecordType: {
    auto *Rec = static_cast<RecordType *>(Type);
    std::vector<llvm::Type *> FieldTypes;
    for (auto &Field : Rec->Fields) {
      if (Field) {
        FieldTypes.push_back(GenerateType(Field->Type.get()));
      }
    }
    Result = llvm::StructType::get(Context, FieldTypes);
    break;
  }
  case NodeKind::DerivedType: {
    auto *Derived = static_cast<DerivedType *>(Type);
    auto It = TypeNameMap.find(ToLower(Derived->ParentType));
    if (It != TypeNameMap.end()) {
      Result = GenerateType(It->second);
    } else {
      Result = llvm::Type::getInt32Ty(Context);
    }
    break;
  }
  case NodeKind::AccessType:
    // Access types lower to opaque pointers.
    Result = llvm::PointerType::get(Context, 0);
    break;
  case NodeKind::ArrayType:
    // TODO: bounded array lowering (P3 exit criteria).
    Result = llvm::PointerType::get(Context, 0);
    break;
  default:
    Result = llvm::Type::getInt32Ty(Context);
    break;
  }

  // Safety net: never hand out null (e.g. an unrecognized scalar kind).
  if (!Result) {
    Result = llvm::Type::getInt32Ty(Context);
  }

  TypeMap[Type] = Result;
  return Result;
}

RecordType *AdaCodeGen::ResolveRecordType(const TypeNode *Type) {
  if (!Type) {
    return nullptr;
  }

  switch (Type->getKind()) {
  case NodeKind::RecordType:
    return static_cast<RecordType *>(const_cast<TypeNode *>(Type));
  case NodeKind::ScalarType: {
    auto *Scalar = static_cast<ScalarType *>(const_cast<TypeNode *>(Type));
    if (!Scalar->Name.empty()) {
      auto It = TypeNameMap.find(ToLower(Scalar->Name));
      if (It != TypeNameMap.end()) {
        return ResolveRecordType(It->second);
      }
    }
    break;
  }
  case NodeKind::DerivedType: {
    auto *Derived =
        static_cast<DerivedType *>(const_cast<TypeNode *>(Type));
    auto It = TypeNameMap.find(ToLower(Derived->ParentType));
    if (It != TypeNameMap.end()) {
      return ResolveRecordType(It->second);
    }
    break;
  }
  default:
    break;
  }
  return nullptr;
}

bool AdaCodeGen::GenerateDeclaration(DeclarationNode *Decl) {
  if (!Decl) {
    return true;
  }

  switch (Decl->getKind()) {
  case NodeKind::ObjectDeclaration: {
    auto *Obj = static_cast<ObjectDeclaration *>(Decl);
    // Package-level variables are not yet supported: they require a
    // current function for the alloca.
    if (!Builder.GetInsertBlock()) {
      return true;
    }
    // Fall back to i32 when the type is missing (parse error recovery)
    // so we never emit an invalid `alloca void`.
    llvm::Type *VarType = Obj->Type ? GenerateType(Obj->Type.get())
                                    : llvm::Type::getInt32Ty(Context);
    std::vector<std::string> Names = Obj->Names;
    if (Names.empty()) {
      Names.push_back(Obj->Name);
    }
    for (const auto &N : Names) {
      llvm::Value *Var = Builder.CreateAlloca(VarType, nullptr, N);
      NameMap[N] = Var;
      VarTypes[N] = Obj->Type.get();
      // Tagged objects initialize their tag slot to the address of the
      // type's tag global (RM 3.9(31)); dispatch compares pointers.
      std::string TypeName = "";
      if (Obj->Type && Obj->Type->getKind() == NodeKind::ScalarType &&
          VarType->isStructTy()) {
        auto *Scalar = static_cast<ScalarType *>(Obj->Type.get());
        if (!Scalar->Name.empty()) {
          TypeName = Scalar->Name;
          auto TGIt = TagGlobals.find(ToLower(Scalar->Name));
          if (TGIt != TagGlobals.end()) {
            llvm::Value *Slot = Builder.CreateStructGEP(
                static_cast<llvm::StructType *>(VarType), Var, 0);
            Builder.CreateStore(TGIt->second, Slot);
          }
        }
      }
      // Controlled types: call Initialize if it exists (RM 3.9.4).
      // Look for an Initialize procedure in scope (local to the subprogram).
      if (!TypeName.empty()) {
        auto InitIt = FunctionMap.find("initialize");
        if (InitIt != FunctionMap.end()) {
          // Call Initialize(Self : access Controlled)
          // For simplicity, pass the object's address directly.
          Builder.CreateCall(InitIt->second, {Var});
        }
        // Track for Finalize at scope exit.
        FinalizeList.push_back({N, TypeName});
      }
      if (Obj->Initializer) {
        llvm::Value *InitVal = GenerateExpression(Obj->Initializer.get());
        if (InitVal) {
          Builder.CreateStore(InitVal, Var);
        }
      }
    }
    break;
  }
  case NodeKind::TaskDeclaration: {
    auto *Task = static_cast<TaskDeclaration *>(Decl);
    // Register the task
    std::string TaskName = Task->Name;
    TaskNames.push_back(TaskName);
    break;
  }
  default:
    // Types and nested subprograms are already registered/pre-declared.
    break;
  }

  return true;
}

void AdaCodeGen::BranchUnlessTerminated(llvm::BasicBlock *Target) {
  llvm::BasicBlock *Cur = Builder.GetInsertBlock();
  if (Cur && !Cur->empty() && Cur->back().isTerminator()) {
    return;
  }
  Builder.CreateBr(Target);
}

void AdaCodeGen::GenerateTryMechanism(
    const std::vector<std::unique_ptr<StatementNode>> &Body,
    const ExceptionHandlerList &Handlers) {
  // P3 skeleton for Ada exceptions (hand-rolled context switch runtime,
  // see ada_runtime.c):
  //
  //   %h = call ptr @__ada_push_handler()      ; handler slot (bookkeeping)
  //   %r = call i32 @__ada_setctx(%h)          ; 0 = first pass, 1 = resume
  //   br i1 %r != 0, dispatch, body
  //   body:   <Body>; pop_handler; br merge
  //   dispatch: match handler names (or re-raise if none matches)
  //   handler_i: <statements>; br merge
  //   merge:   (continues after the try)
  //
  // The context save must be a direct call from the function that owns the
  // try so the saved return address points into a live frame (a wrapper
  // that returns before the raise leaves a dead resume target). Only %r
  // may be used after the setctx call: on resume the slot pointer is
  // clobbered, but the call chain keeps push_handler ordered before it.
  // The except path emits no pop (stale-slot scheme; see ada_runtime.c).
  llvm::Function *F = Builder.GetInsertBlock()->getParent();
  auto *BodyBB = llvm::BasicBlock::Create(Context, "try.body", F);
  auto *DispatchBB = llvm::BasicBlock::Create(Context, "try.dispatch", F);
  auto *MergeBB = llvm::BasicBlock::Create(Context, "try.end", F);

  llvm::Type *IntTy = llvm::Type::getInt32Ty(Context);
  llvm::Type *PtrTy = llvm::PointerType::get(Context, 0);
  llvm::FunctionCallee PushFn = Module->getOrInsertFunction(
      "__ada_push_handler", llvm::FunctionType::get(PtrTy, {}, false));
  llvm::FunctionCallee SetCtxFn = Module->getOrInsertFunction(
      "__ada_setctx", llvm::FunctionType::get(IntTy, {PtrTy}, false));
  llvm::FunctionCallee PopFn = Module->getOrInsertFunction(
      "__ada_pop_handler", llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {}, false));
  llvm::FunctionCallee RaiseFn = Module->getOrInsertFunction(
      "__ada_raise", llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {PtrTy}, false));
  llvm::FunctionCallee CurFn = Module->getOrInsertFunction(
      "__ada_current_exception", llvm::FunctionType::get(PtrTy, {}, false));
  llvm::FunctionCallee MatchFn = Module->getOrInsertFunction(
      "__ada_exception_match", llvm::FunctionType::get(IntTy, {PtrTy}, false));

  llvm::Value *H = Builder.CreateCall(PushFn, {}, "try.h");
  llvm::Value *R = Builder.CreateCall(SetCtxFn, {H}, "try.r");
  llvm::Value *T = Builder.CreateICmpNE(R, Builder.getInt32(0));
  Builder.CreateCondBr(T, DispatchBB, BodyBB);

  Builder.SetInsertPoint(BodyBB);
  GenerateStatements(Body);
  // A `return` in the body already terminates BodyBB; in that case the
  // handler stays on the stack (P3 limitation, see decisions log). Check
  // the current insertion block, not BodyBB, because the body may end with
  // a tagged dispatch call that leaves the insertion point at an
  // unterminated dispatch.result block.
  llvm::BasicBlock *InsertBB = Builder.GetInsertBlock();
  if (InsertBB && (InsertBB->empty() || !InsertBB->back().isTerminator())) {
    Builder.CreateCall(PopFn);
    Builder.CreateBr(MergeBB);
  }

  // Handler dispatch chain: one match block between handlers plus a
  // fall-through re-raise block when no handler matches (propagate to an
  // outer handler or the runtime top level).
  size_t NumHandlers = Handlers.size();
  if (NumHandlers > 1024) NumHandlers = 1024;
  llvm::BasicBlock *DispatchBBs[1024] = {};
  for (size_t I = 1; I < NumHandlers; ++I) {
    DispatchBBs[I] =
        llvm::BasicBlock::Create(Context, "try.match" + std::to_string(I), F);
  }
  llvm::BasicBlock *HandlerBBs[1024];
  for (size_t I = 0; I < NumHandlers; ++I) {
    HandlerBBs[I] =
        llvm::BasicBlock::Create(Context, "try.handler" + std::to_string(I), F);
  }
  llvm::BasicBlock *ReraiseBB =
      llvm::BasicBlock::Create(Context, "try.reraise", F);

  llvm::BasicBlock *Cur = DispatchBB;
  for (size_t I = 0; I < NumHandlers; ++I) {
    Builder.SetInsertPoint(Cur);
    if (Handlers[I].first.empty() || Handlers[I].first == "others") {
      Builder.CreateBr(HandlerBBs[I]);
    } else {
      llvm::Constant *NameC = Builder.CreateGlobalString(Handlers[I].first);
      llvm::Value *M = Builder.CreateCall(MatchFn, {NameC}, "try.match");
      llvm::Value *C = Builder.CreateICmpNE(M, Builder.getInt32(0));
      llvm::BasicBlock *Next =
          (I + 1 < NumHandlers) ? DispatchBBs[I + 1] : ReraiseBB;
      Builder.CreateCondBr(C, HandlerBBs[I], Next);
    }
    Cur = (I + 1 < NumHandlers) ? DispatchBBs[I + 1] : nullptr;
  }

  // No handler matched: re-raise to an outer handler. (Dead block when an
  // `others` handler is present; legal IR either way.)
  Builder.SetInsertPoint(ReraiseBB);
  {
    llvm::Value *N = Builder.CreateCall(CurFn, {}, "try.name");
    Builder.CreateCall(RaiseFn, {N});
    Builder.CreateUnreachable();
  }

  for (size_t I = 0; I < NumHandlers; ++I) {
    Builder.SetInsertPoint(HandlerBBs[I]);
    GenerateStatements(Handlers[I].second);
    BranchUnlessTerminated(MergeBB);
  }

  Builder.SetInsertPoint(MergeBB);
}

void AdaCodeGen::GenerateStatements(
    const std::vector<std::unique_ptr<StatementNode>> &Stmts) {
  for (const auto &Stmt : Stmts) {
    if (Stmt) {
      GenerateStatement(Stmt.get());
    }
  }
}

void AdaCodeGen::EmitContractViolation() {
  // Raise Constraint_Error through the runtime hook (same pattern as
  // RaiseStatement), then mark the path noreturn.
  llvm::Type *PtrTy = llvm::PointerType::get(Context, 0);
  llvm::FunctionCallee RaiseFn = Module->getOrInsertFunction(
      "__ada_raise",
      llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {PtrTy}, false));
  llvm::Constant *NameC = Builder.CreateGlobalString("Constraint_Error");
  Builder.CreateCall(RaiseFn, {NameC});
  // A failed contract never returns; the raise propagates to the caller.
  Builder.CreateUnreachable();
}

void AdaCodeGen::GenerateStatement(StatementNode *Stmt) {
  if (!Stmt) {
    return;
  }

  switch (Stmt->getKind()) {
  case NodeKind::ReturnStatement: {
    auto *RetStmt = static_cast<ReturnStatement *>(Stmt);
    if (RetStmt->Value) {
      llvm::Value *RetVal = GenerateExpression(RetStmt->Value.get());
      if (RetVal) {
        // Postcondition assertions (RM 5.5.1(16)): bind 'Result to the return
        // value and evaluate each Post aspect before returning.
        if (ContractsEnabled && CurrentDecl && CurrentResult) {
          bool HasPost = false;
          for (auto &A : CurrentDecl->Aspects) {
            if (A && A->AspectName == "Post" && A->AspectValue) {
              HasPost = true;
              break;
            }
          }
          if (HasPost) {
            llvm::Function *Func = Builder.GetInsertBlock()->getParent();
            Builder.CreateStore(RetVal, CurrentResult);
            for (auto &A : CurrentDecl->Aspects) {
              if (!A || A->AspectName != "Post" || !A->AspectValue)
                continue;
              llvm::Value *Cond = GenerateExpression(A->AspectValue.get());
              if (!Cond || !Cond->getType()->isIntegerTy(1))
                continue;
              llvm::BasicBlock *FailBB =
                  llvm::BasicBlock::Create(Context, "post.fail", Func);
              llvm::BasicBlock *OkBB =
                  llvm::BasicBlock::Create(Context, "post.ok", Func);
              Builder.CreateCondBr(Cond, OkBB, FailBB);
              Builder.SetInsertPoint(FailBB);
              EmitContractViolation();
              Builder.SetInsertPoint(OkBB);
            }
          }
        }
        Builder.CreateRet(RetVal);
      } else {
        Builder.CreateRetVoid();
      }
    } else {
      Builder.CreateRetVoid();
    }
    break;
  }
  case NodeKind::AssignmentStatement: {
    auto *Assign = static_cast<AssignmentStatement *>(Stmt);
    if (!Assign->Target || !Assign->Value) {
      break;
    }
    // Tagged assignment (RM 3.9.4): both sides are objects of tagged
    // types; tags must be identical, else raise Storage_Error.
    if (Assign->Target->getKind() == NodeKind::IdentifierExpression &&
        Assign->Value->getKind() == NodeKind::IdentifierExpression) {
      auto *TIdent = static_cast<IdentifierExpression *>(Assign->Target.get());
      auto *VIdent = static_cast<IdentifierExpression *>(Assign->Value.get());
      auto TIt = VarTypes.find(TIdent->Name);
      auto VIt = VarTypes.find(VIdent->Name);
      if (TIt != VarTypes.end() && VIt != VarTypes.end() &&
          TIt->second && TIt->second->getKind() == NodeKind::ScalarType &&
          VIt->second && VIt->second->getKind() == NodeKind::ScalarType) {
        std::string TName =
            ToLower(static_cast<ScalarType *>(TIt->second)->Name);
        std::string VName =
            ToLower(static_cast<ScalarType *>(VIt->second)->Name);
        if (TaggedLayouts.count(TName) && TaggedLayouts.count(VName)) {
          llvm::Type *Dummy;
          llvm::Value *T = GenerateLValue(Assign->Target.get(), Dummy);
          llvm::Value *V = GenerateLValue(Assign->Value.get(), Dummy);
          if (T && V && GenerateTaggedAssignment(T, V, TName, VName)) {
            break;
          }
        }
      }
    }
    llvm::Type *ElemType;
    llvm::Value *Target = GenerateLValue(Assign->Target.get(), ElemType);
    if (!Target) {
      break;
    }
    llvm::Value *Val = GenerateExpression(Assign->Value.get());
    if (Val) {
      Builder.CreateStore(Val, Target);
    }
    break;
  }
  case NodeKind::NullStatement:
    break;
  case NodeKind::DelayStatement: {
    auto *Delay = static_cast<DelayStatement *>(Stmt);
    if (Delay->Duration) {
      llvm::Value *Duration = GenerateExpression(Delay->Duration.get());
      if (Duration) {
        // Call __ada_delay(duration) runtime function
        llvm::FunctionCallee DelayFn = Module->getOrInsertFunction(
            "__ada_delay",
            llvm::FunctionType::get(Builder.getVoidTy(),
                                    {llvm::Type::getDoubleTy(Context)},
                                    false));
        Builder.CreateCall(DelayFn, {Duration});
      }
    }
    break;
  }
  case NodeKind::RequeueStatement: {
    auto *Requeue = static_cast<RequeueStatement *>(Stmt);
    // P10: requeue semantics - signal the task to requeue on the specified entry
    // This is a simplified implementation; full requeue with abort semantics
    // would require more complex runtime support.
    // For now, we just call a runtime stub that logs the requeue operation.
    auto *PtrTy = llvm::PointerType::get(Context, 0);
    llvm::FunctionCallee RequeueFn = Module->getOrInsertFunction(
        "__ada_requeue",
        llvm::FunctionType::get(Builder.getVoidTy(), {PtrTy}, false));
    llvm::Constant *EntryName = Builder.CreateGlobalString(Requeue->EntryName);
    Builder.CreateCall(RequeueFn, {EntryName});
    break;
  }
  case NodeKind::IfStatement: {
    auto *If = static_cast<IfStatement *>(Stmt);
    if (!If->Condition) {
      break;
    }
    llvm::Function *F = Builder.GetInsertBlock()->getParent();

    llvm::Value *Cond = GenerateExpression(If->Condition.get());
    if (!Cond) {
      break;
    }
    auto *ThenBB = llvm::BasicBlock::Create(Context, "if.then", F);
    auto *NextBB = llvm::BasicBlock::Create(Context, "if.else", F);
    Builder.CreateCondBr(Cond, ThenBB, NextBB);

    // Branch bodies that may fall through to the merge block.
    std::vector<llvm::BasicBlock *> BranchBBs;
    BranchBBs.push_back(ThenBB);
    Builder.SetInsertPoint(ThenBB);
    GenerateStatements(If->ThenStatements);

    Builder.SetInsertPoint(NextBB);
    for (auto &Elsif : If->Elsifs) {
      llvm::Value *ECond = GenerateExpression(Elsif.first.get());
      if (!ECond) {
        break;
      }
      auto *EThenBB = llvm::BasicBlock::Create(Context, "if.then", F);
      auto *ENextBB = llvm::BasicBlock::Create(Context, "if.else", F);
      Builder.CreateCondBr(ECond, EThenBB, ENextBB);
      BranchBBs.push_back(EThenBB);
      Builder.SetInsertPoint(EThenBB);
      GenerateStatements(Elsif.second);
      Builder.SetInsertPoint(ENextBB);
    }
    GenerateStatements(If->ElseStatements);
    BranchBBs.push_back(NextBB);

    // The merge block is created last so it is the function's tail block at
    // this point; the following statements (or the epilogue return)
    // terminate it. A branch that already ends with a `return` is skipped.
    auto *MergeBB = llvm::BasicBlock::Create(Context, "if.end", F);
    for (auto *BB : BranchBBs) {
      Builder.SetInsertPoint(BB);
      BranchUnlessTerminated(MergeBB);
    }
    Builder.SetInsertPoint(MergeBB);
    break;
  }
  case NodeKind::WhileLoopStatement: {
    auto *While = static_cast<WhileLoopStatement *>(Stmt);
    if (!While->Condition) {
      break;
    }
    llvm::Function *F = Builder.GetInsertBlock()->getParent();
    auto *CondBB = llvm::BasicBlock::Create(Context, "while.cond", F);
    auto *BodyBB = llvm::BasicBlock::Create(Context, "while.body", F);
    auto *MergeBB = llvm::BasicBlock::Create(Context, "while.end", F);
    Builder.CreateBr(CondBB);

    Builder.SetInsertPoint(CondBB);
    llvm::Value *Cond = GenerateExpression(While->Condition.get());
    Builder.CreateCondBr(Cond, BodyBB, MergeBB);

    Builder.SetInsertPoint(BodyBB);
    LoopExits.push_back(MergeBB);
    GenerateStatements(While->Body);
    BranchUnlessTerminated(CondBB);
    LoopExits.pop_back();

    Builder.SetInsertPoint(MergeBB);
    break;
  }
  case NodeKind::ForLoopStatement: {
    auto *ForLoop = static_cast<ForLoopStatement *>(Stmt);
    llvm::Function *F = Builder.GetInsertBlock()->getParent();
    llvm::Type *IntTy = llvm::Type::getInt32Ty(Context);
    llvm::Value *Var = Builder.CreateAlloca(IntTy, nullptr, ForLoop->Variable);
    llvm::Value *Low = GenerateExpression(ForLoop->Low.get());
    llvm::Value *High = GenerateExpression(ForLoop->High.get());
    if (!Low || !High) {
      break;
    }
    Builder.CreateStore(Low, Var);

    auto *CondBB = llvm::BasicBlock::Create(Context, "for.cond", F);
    auto *BodyBB = llvm::BasicBlock::Create(Context, "for.body", F);
    auto *MergeBB = llvm::BasicBlock::Create(Context, "for.end", F);
    Builder.CreateBr(CondBB);

    Builder.SetInsertPoint(CondBB);
    llvm::Value *Cur = Builder.CreateLoad(IntTy, Var, "for.i");
    llvm::Value *Cond = ForLoop->IsReverse
                            ? Builder.CreateICmpUGE(Cur, High, "for.cmp")
                            : Builder.CreateICmpULE(Cur, High, "for.cmp");
    Builder.CreateCondBr(Cond, BodyBB, MergeBB);

    Builder.SetInsertPoint(BodyBB);
    NameMap[ForLoop->Variable] = Var;
    LoopExits.push_back(MergeBB);
    GenerateStatements(ForLoop->Body);
    LoopExits.pop_back();
    // The body may end with a `return`; only emit the increment/loop-back
    // when the block is not already terminated.
    if (BodyBB->empty() || !BodyBB->back().isTerminator()) {
      llvm::Value *Step = ForLoop->IsReverse
                              ? Builder.CreateSub(Cur, Builder.getInt32(1), "for.next")
                              : Builder.CreateAdd(Cur, Builder.getInt32(1), "for.next");
      Builder.CreateStore(Step, Var);
      Builder.CreateBr(CondBB);
    }

    Builder.SetInsertPoint(MergeBB);
    break;
  }
  case NodeKind::LoopStatement: {
    auto *Loop = static_cast<LoopStatement *>(Stmt);
    llvm::Function *F = Builder.GetInsertBlock()->getParent();
    auto *BodyBB = llvm::BasicBlock::Create(Context, "loop.body", F);
    auto *MergeBB = llvm::BasicBlock::Create(Context, "loop.end", F);
    Builder.CreateBr(BodyBB);
    LoopExits.push_back(MergeBB);
    Builder.SetInsertPoint(BodyBB);
    GenerateStatements(Loop->Body);
    BranchUnlessTerminated(BodyBB);
    LoopExits.pop_back();
    Builder.SetInsertPoint(MergeBB);
    break;
  }
  case NodeKind::ExitStatement: {
    auto *Exit = static_cast<ExitStatement *>(Stmt);
    if (LoopExits.empty()) {
      break;
    }
    llvm::Value *Cond = Exit->Condition
                            ? GenerateExpression(Exit->Condition.get())
                            : nullptr;
    if (Cond) {
      llvm::Function *F = Builder.GetInsertBlock()->getParent();
      auto *NextBB = llvm::BasicBlock::Create(Context, "exit.next", F);
      Builder.CreateCondBr(Cond, LoopExits.back(), NextBB);
      Builder.SetInsertPoint(NextBB);
    } else {
      Builder.CreateBr(LoopExits.back());
    }
    break;
  }
  case NodeKind::RaiseStatement: {
    auto *Raise = static_cast<RaiseStatement *>(Stmt);
    // P3 skeleton: exceptions are identified by name only (the runtime
    // carries a name string, not an exception object).
    std::string Name = "Program_Error";
    if (Raise->Exception &&
        Raise->Exception->getKind() == NodeKind::IdentifierExpression) {
      Name =
          static_cast<IdentifierExpression *>(Raise->Exception.get())->Name;
    }
    llvm::Type *PtrTy = llvm::PointerType::get(Context, 0);
    llvm::FunctionCallee RaiseFn = Module->getOrInsertFunction(
        "__ada_raise",
        llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {PtrTy},
                                false));
    llvm::Constant *NameC = Builder.CreateGlobalString(Name);
    Builder.CreateCall(RaiseFn, {NameC});
    break;
  }
  case NodeKind::TryStatement: {
    auto *Try = static_cast<TryStatement *>(Stmt);
    GenerateTryMechanism(Try->Body, Try->Handlers);
    break;
  }
  case NodeKind::ProcedureCallStatement: {
    auto *PCall = static_cast<ProcedureCallStatement *>(Stmt);
    if (!PCall->Procedure) {
      break;
    }
    ExpressionNode *Proc = PCall->Procedure.get();
    if (Proc && Proc->getKind() == NodeKind::CallExpression) {
      // The parser consumes `Name(args)` into the call expression itself.
      GenerateCall(static_cast<CallExpression *>(Proc));
    } else if (Proc && Proc->getKind() == NodeKind::IdentifierExpression) {
      auto *Ident = static_cast<IdentifierExpression *>(Proc);
      std::string FnName = ToLower(Ident->Name);
      // Special case: Ada I/O and other runtime builtins.
      llvm::Value *Unused = nullptr;
      if (GenerateRuntimeCall(FnName, PCall->Arguments, Unused)) {
        // Handled by the runtime.
      } else {
        // Bare identifier: resolve the callee and generate the call.
        auto It = FunctionMap.find(ToLower(Ident->Name));
        if (It != FunctionMap.end()) {
          std::vector<llvm::Value *> Args;
          bool Ok = true;
          for (auto &Arg : PCall->Arguments) {
            llvm::Value *V = GenerateExpression(Arg.get());
            if (!V) {
              Ok = false;
              break;
            }
            Args.push_back(V);
          }
          if (Ok) {
            Builder.CreateCall(It->second, Args);
          }
        }
      }
      break;
    } else if (Proc && Proc->getKind() == NodeKind::RecordFieldAccess) {
      // Entry call: TaskName.EntryName(args)
      // Generate call to runtime entry call function (blocks until accept completes)
      auto *FA = static_cast<RecordFieldAccess *>(Proc);
      std::string TaskName = "";
      if (FA->Record && FA->Record->getKind() == NodeKind::IdentifierExpression) {
        TaskName = static_cast<IdentifierExpression *>(FA->Record.get())->Name;
      }
      std::string EntryName = FA->FieldName;
      
      // Look up the task object in NameMap
      auto It = NameMap.find(TaskName);
      if (It != NameMap.end()) {
        llvm::Value *Task = It->second;
        auto *PtrTy = llvm::PointerType::get(Context, 0);
        auto *IntTy = llvm::Type::getInt32Ty(Context);
        
        // Generate code for entry call arguments
        std::vector<llvm::Value *> ArgValues;
        for (auto &Arg : PCall->Arguments) {
          llvm::Value *V = GenerateExpression(Arg.get());
          if (V) {
            ArgValues.push_back(V);
          }
        }
        
        // For now, pass arguments through runtime's global parameter array
        // Store each argument in the array
        if (!ArgValues.empty()) {
          for (size_t i = 0; i < ArgValues.size(); i++) {
            llvm::Value *Idx = Builder.getInt32(i);
            // Store in __ada_entry_params array
            llvm::GlobalVariable *Params = Module->getOrInsertGlobal(
                "__ada_entry_params",
                llvm::ArrayType::get(IntTy, 10));
            llvm::Value *GEP = Builder.CreateGEP(IntTy, Params, {Builder.getInt32(0), Idx}, "entry_param");
            Builder.CreateStore(ArgValues[i], GEP);
          }
        }
        
        // Call __ada_entry_call(task, num_params, params)
        llvm::FunctionCallee EntryCallFn = Module->getOrInsertFunction(
            "__ada_entry_call",
            llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {PtrTy, IntTy, PtrTy}, false));
        
        llvm::Value *NumParams = Builder.getInt32(ArgValues.size());
        llvm::GlobalVariable *Params = Module->getOrInsertGlobal(
            "__ada_entry_params",
            llvm::ArrayType::get(IntTy, 10));
        llvm::Value *ParamsPtr = Builder.CreatePointerCast(Params, PtrTy, "entry_params_ptr");
        
        Builder.CreateCall(EntryCallFn, {Task, NumParams, ParamsPtr});
      }
      break;
    }
    break;
  }
  case NodeKind::AcceptStatement: {
    auto *Accept = static_cast<AcceptStatement *>(Stmt);
    // Generate: call __ada_entry_accept_start(task)
    // Bind parameters from runtime array to local variables
    // Generate accept body
    // Generate: call __ada_entry_accept_end(task)
    
    // Look up the task object in NameMap
    if (CurrentTask) {
      auto It = NameMap.find(CurrentTask->Name);
      if (It != NameMap.end()) {
        llvm::Value *Task = It->second;
        
        // Call __ada_entry_accept_start(task) - blocks until entry call arrives
        auto *PtrTy = llvm::PointerType::get(Context, 0);
        auto *IntTy = llvm::Type::getInt32Ty(Context);
        llvm::FunctionCallee AcceptStartFn = Module->getOrInsertFunction(
            "__ada_entry_accept_start",
            llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {PtrTy}, false));
        Builder.CreateCall(AcceptStartFn, {Task});
        
        // Bind parameters from runtime array to local variables
        for (size_t i = 0; i < Accept->FormalParameters.size(); i++) {
          auto *Param = Accept->FormalParameters[i].get();
          // Load value from __ada_entry_params[i]
          llvm::GlobalVariable *Params = Module->getOrInsertGlobal(
              "__ada_entry_params",
              llvm::ArrayType::get(IntTy, 10));
          llvm::Value *Idx = Builder.getInt32(i);
          llvm::Value *GEP = Builder.CreateGEP(IntTy, Params, {Builder.getInt32(0), Idx}, "accept_param");
          llvm::Value *Val = Builder.CreateLoad(IntTy, GEP, Param->Name);
          
          // Store in local variable (NameMap)
          NameMap[Param->Name] = Val;
        }
        
        // Generate accept body
        GenerateStatements(Accept->Body);
        
        // Call __ada_entry_accept_end(task) - signals caller that accept completed
        llvm::FunctionCallee AcceptEndFn = Module->getOrInsertFunction(
            "__ada_entry_accept_end",
            llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {PtrTy}, false));
        Builder.CreateCall(AcceptEndFn, {Task});
      } else {
        // Task not found, just generate the body
        GenerateStatements(Accept->Body);
      }
    } else {
      // No current task, just generate the body
      GenerateStatements(Accept->Body);
    }
    break;
  }
  default:
    break;
  }
}

llvm::Value *
AdaCodeGen::GenerateLValue(const ExpressionNode *Expr, llvm::Type *&ElemType) {
  if (!Expr) {
    return nullptr;
  }

  switch (Expr->getKind()) {
  case NodeKind::IdentifierExpression: {
    auto *Ident = static_cast<const IdentifierExpression *>(Expr);
    auto It = NameMap.find(Ident->Name);
    if (It == NameMap.end()) {
      return nullptr;
    }
    if (auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(It->second)) {
      ElemType = Alloca->getAllocatedType();
    } else {
      ElemType = It->second->getType();
    }
    return It->second;
  }
  case NodeKind::RecordFieldAccess: {
    auto *FA = static_cast<const RecordFieldAccess *>(Expr);
    llvm::Type *BaseTy;
    llvm::Value *BasePtr = GenerateLValue(FA->Record.get(), BaseTy);
    if (!BasePtr) {
      return nullptr;
    }
    // Determine the record type of the base object.
    RecordType *RecType = nullptr;
    ExpressionNode *Base = FA->Record.get();
    if (Base && Base->getKind() == NodeKind::IdentifierExpression) {
      auto *BaseIdent = static_cast<IdentifierExpression *>(Base);
      auto Vit = VarTypes.find(BaseIdent->Name);
      if (Vit != VarTypes.end()) {
        // Tagged field access: resolve against the tagged layout (fields
        // follow the tag slot, struct index 1 + position).
        if (Vit->second &&
            Vit->second->getKind() == NodeKind::ScalarType) {
          auto *ST = static_cast<ScalarType *>(Vit->second);
          std::string Key = ToLower(ST->Name);
          auto LT = TaggedLayouts.find(Key);
          if (LT != TaggedLayouts.end()) {
            for (size_t I = 0; I < LT->second.size(); I++) {
              if (ToLower(LT->second[I].first) == ToLower(FA->FieldName)) {
                std::vector<llvm::Value *> Indices = {
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 0),
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context),
                                           I + 1)};
                ElemType = LT->second[I].second;
                return Builder.CreateGEP(TaggedStructType(Key), BasePtr,
                                         Indices, FA->FieldName);
              }
            }
            return nullptr;
          }
        }
        RecType = ResolveRecordType(Vit->second);
      }
    }
    if (!RecType) {
      return nullptr;
    }
    // Find the field index.
    size_t FieldIndex = RecType->Fields.size();
    for (size_t I = 0; I < RecType->Fields.size(); I++) {
      if (RecType->Fields[I] &&
          ToLower(RecType->Fields[I]->Name) == ToLower(FA->FieldName)) {
        FieldIndex = I;
        break;
      }
    }
    if (FieldIndex >= RecType->Fields.size()) {
      return nullptr;
    }
    llvm::Type *STy = GenerateType(const_cast<TypeNode *>(
        static_cast<const TypeNode *>(RecType)));
    ElemType = GenerateType(const_cast<TypeNode *>(
        RecType->Fields[FieldIndex]->Type.get()));
    std::vector<llvm::Value *> Indices = {
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), FieldIndex)};
    return Builder.CreateGEP(STy, BasePtr, Indices, FA->FieldName);
  }
  default:
    return nullptr;
  }
}

bool AdaCodeGen::GenerateRuntimeCall(
    const std::string &FnName,
    const std::vector<std::unique_ptr<ExpressionNode>> &Args,
    llvm::Value *&Result) {
  Result = nullptr;
  auto *PtrTy = llvm::PointerType::get(Context, 0);
  auto *I32Ty = llvm::Type::getInt32Ty(Context);
  auto *DoubleTy = llvm::Type::getDoubleTy(Context);
  auto *VoidTy = llvm::Type::getVoidTy(Context);

  // Text_IO: Put / Put_Line are overloaded on the argument type.
  if (FnName == "put" && !Args.empty()) {
    llvm::Value *Arg = GenerateExpression(Args[0].get());
    if (Arg) {
      if (Arg->getType() == PtrTy) {
        llvm::FunctionCallee Fn = Module->getOrInsertFunction(
            "__ada_put", llvm::FunctionType::get(VoidTy, {PtrTy}, false));
        Builder.CreateCall(Fn, {Arg});
      } else if (Arg->getType() == I32Ty) {
        llvm::FunctionCallee Fn = Module->getOrInsertFunction(
            "__ada_print_int", llvm::FunctionType::get(VoidTy, {I32Ty}, false));
        Builder.CreateCall(Fn, {Arg});
      } else if (Arg->getType() == DoubleTy) {
        llvm::FunctionCallee Fn = Module->getOrInsertFunction(
            "__ada_print_float",
            llvm::FunctionType::get(VoidTy, {DoubleTy}, false));
        Builder.CreateCall(Fn, {Arg});
      }
    }
    return true;
  }
  if (FnName == "put_line" && !Args.empty()) {
    llvm::Value *Arg = GenerateExpression(Args[0].get());
    if (Arg) {
      if (Arg->getType() == PtrTy) {
        llvm::FunctionCallee Fn = Module->getOrInsertFunction(
            "__ada_print", llvm::FunctionType::get(VoidTy, {PtrTy}, false));
        Builder.CreateCall(Fn, {Arg});
      } else if (Arg->getType() == I32Ty) {
        llvm::FunctionCallee Fn = Module->getOrInsertFunction(
            "__ada_print_int_new_line",
            llvm::FunctionType::get(VoidTy, {I32Ty}, false));
        Builder.CreateCall(Fn, {Arg});
      } else if (Arg->getType() == DoubleTy) {
        llvm::FunctionCallee Fn = Module->getOrInsertFunction(
            "__ada_print_float_new_line",
            llvm::FunctionType::get(VoidTy, {DoubleTy}, false));
        Builder.CreateCall(Fn, {Arg});
      }
    }
    return true;
  }
  if (FnName == "new_line") {
    llvm::FunctionCallee Fn = Module->getOrInsertFunction(
        "__ada_new_line", llvm::FunctionType::get(VoidTy, {}, false));
    Builder.CreateCall(Fn, {});
    return true;
  }
  if (FnName == "print_int" && !Args.empty()) {
    llvm::FunctionCallee Fn = Module->getOrInsertFunction(
        "__ada_print_int", llvm::FunctionType::get(VoidTy, {I32Ty}, false));
    llvm::Value *Arg = GenerateExpression(Args[0].get());
    if (Arg && Arg->getType() == I32Ty)
      Builder.CreateCall(Fn, {Arg});
    return true;
  }
  if (FnName == "print_float" && !Args.empty()) {
    llvm::FunctionCallee Fn = Module->getOrInsertFunction(
        "__ada_print_float",
        llvm::FunctionType::get(VoidTy, {DoubleTy}, false));
    llvm::Value *Arg = GenerateExpression(Args[0].get());
    if (Arg && Arg->getType() == DoubleTy)
      Builder.CreateCall(Fn, {Arg});
    return true;
  }
  if (FnName == "print_int_new_line" && !Args.empty()) {
    llvm::FunctionCallee Fn = Module->getOrInsertFunction(
        "__ada_print_int_new_line",
        llvm::FunctionType::get(VoidTy, {I32Ty}, false));
    llvm::Value *Arg = GenerateExpression(Args[0].get());
    if (Arg && Arg->getType() == I32Ty)
      Builder.CreateCall(Fn, {Arg});
    return true;
  }
  if (FnName == "print_float_new_line" && !Args.empty()) {
    llvm::FunctionCallee Fn = Module->getOrInsertFunction(
        "__ada_print_float_new_line",
        llvm::FunctionType::get(VoidTy, {DoubleTy}, false));
    llvm::Value *Arg = GenerateExpression(Args[0].get());
    if (Arg && Arg->getType() == DoubleTy)
      Builder.CreateCall(Fn, {Arg});
    return true;
  }
  if (FnName == "get_int" && !Args.empty()) {
    llvm::Type *ElemType;
    llvm::Value *Target = GenerateLValue(Args[0].get(), ElemType);
    if (Target && ElemType == I32Ty) {
      llvm::FunctionCallee Fn = Module->getOrInsertFunction(
          "__ada_integer_get", llvm::FunctionType::get(VoidTy, {PtrTy}, false));
      Builder.CreateCall(Fn, {Target});
    }
    return true;
  }
  if (FnName == "get_float" && !Args.empty()) {
    llvm::Type *ElemType;
    llvm::Value *Target = GenerateLValue(Args[0].get(), ElemType);
    if (Target && ElemType == DoubleTy) {
      llvm::FunctionCallee Fn = Module->getOrInsertFunction(
          "__ada_float_get", llvm::FunctionType::get(VoidTy, {PtrTy}, false));
      Builder.CreateCall(Fn, {Target});
    }
    return true;
  }
  // Ada.Calendar.Clock -> seconds since the epoch (Double).
  if (FnName == "clock") {
    llvm::FunctionCallee Fn = Module->getOrInsertFunction(
        "__ada_clock", llvm::FunctionType::get(DoubleTy, {}, false));
    Result = Builder.CreateCall(Fn, {});
    return true;
  }
  // Ada.Strings.Fixed.Length -> Integer.
  if ((FnName == "length" || FnName == "string_length") && !Args.empty()) {
    llvm::FunctionCallee Fn = Module->getOrInsertFunction(
        "__ada_string_length", llvm::FunctionType::get(I32Ty, {PtrTy}, false));
    llvm::Value *Arg = GenerateExpression(Args[0].get());
    if (Arg && Arg->getType() == PtrTy)
      Result = Builder.CreateCall(Fn, {Arg});
    return true;
  }
  // Ada.Environment_Variables.Value / Exists (also Ada.Environment_Variables
  // is resolved to the bare selected names Value/Exists).
  if ((FnName == "env_value" || FnName == "environment_value" ||
       FnName == "value") &&
      !Args.empty()) {
    llvm::FunctionCallee Fn = Module->getOrInsertFunction(
        "__ada_env_value", llvm::FunctionType::get(PtrTy, {PtrTy}, false));
    llvm::Value *Arg = GenerateExpression(Args[0].get());
    if (Arg && Arg->getType() == PtrTy)
      Result = Builder.CreateCall(Fn, {Arg});
    return true;
  }
  if ((FnName == "env_exists" || FnName == "environment_exists" ||
       FnName == "exists") &&
      !Args.empty()) {
    llvm::FunctionCallee Fn = Module->getOrInsertFunction(
        "__ada_env_exists", llvm::FunctionType::get(I32Ty, {PtrTy}, false));
    llvm::Value *Arg = GenerateExpression(Args[0].get());
    if (Arg && Arg->getType() == PtrTy) {
      llvm::Value *Rc = Builder.CreateCall(Fn, {Arg});
      Result = Builder.CreateICmpNE(Rc, llvm::ConstantInt::get(I32Ty, 0),
                                    "exists");
    }
    return true;
  }
  return false;
}

llvm::Value *AdaCodeGen::GenerateCall(CallExpression *Call) {
  if (!Call || !Call->Function) {
    return nullptr;
  }

  ExpressionNode *Fn = Call->Function.get();
  if (!Fn || Fn->getKind() != NodeKind::IdentifierExpression) {
    return nullptr;
  }
  std::string FnName = ToLower(static_cast<IdentifierExpression *>(Fn)->Name);

  // Runtime-library builtins take priority over user subprograms for the
  // well-known I/O/calendar/strings names.
  llvm::Value *BuiltinValue = nullptr;
  if (GenerateRuntimeCall(FnName, Call->Arguments, BuiltinValue))
    return BuiltinValue;

  // Resolve the static callee and (for dispatch) its declaration.
  llvm::Function *Callee = nullptr;
  SubprogramDeclaration *CalleeDecl = nullptr;
  if (Call->Resolved) {
    auto DIt = DeclToFunction.find(Call->Resolved);
    if (DIt != DeclToFunction.end()) {
      Callee = DIt->second;
      CalleeDecl = Call->Resolved;
    }
  }
  if (!Callee) {
    for (auto It = Subprograms.rbegin(); It != Subprograms.rend(); ++It) {
      if (ToLower(MangledName(*It)) == FnName) {
        auto DIt = DeclToFunction.find(*It);
        if (DIt != DeclToFunction.end()) {
          Callee = DIt->second;
          CalleeDecl = *It;
          break;
        }
      }
    }
  }
  if (!Callee) {
    auto FIt = FunctionMap.find(FnName);
    if (FIt != FunctionMap.end()) {
      Callee = FIt->second;
    }
  }
  if (!Callee) {
    // Not a known subprogram (external or runtime call); leave unresolved.
    std::cerr << "AdaCodeGen: cannot resolve call to '" << FnName << "'\n";
    return nullptr;
  }
  if (!CalleeDecl) {
    for (auto It = DeclToFunction.rbegin(); It != DeclToFunction.rend();
         ++It) {
      if (It->second == Callee) {
        CalleeDecl = It->first;
        break;
      }
    }
  }

  // Arguments: by-reference parameters (pointers in the LLVM type) take the
  // variable's pointer itself; everything else is a loaded value.
  std::vector<llvm::Value *> Args;
  auto *FTy = Callee->getFunctionType();
  for (size_t I = 0; I < Call->Arguments.size(); I++) {
    bool ByRef = I < FTy->getNumParams() &&
                 FTy->getParamType(I)->isPointerTy();
    ExpressionNode *Arg = Call->Arguments[I].get();
    llvm::Value *V = nullptr;
    if (ByRef && Arg && Arg->getKind() == NodeKind::IdentifierExpression) {
      auto *AIdent = static_cast<IdentifierExpression *>(Arg);
      auto NIt = NameMap.find(AIdent->Name);
      if (NIt != NameMap.end()) {
        V = NIt->second;
      }
    }
    if (!V) {
      V = GenerateExpression(Arg);
      if (!V) {
        return nullptr;
      }
    }
    Args.push_back(V);
  }

  llvm::Value *Result = nullptr;
  if (GenerateTaggedDispatch(Call, Callee, CalleeDecl, Args, Result)) {
    return Result;
  }
  return Builder.CreateCall(Callee, Args);
}

llvm::Value *AdaCodeGen::GenerateExpression(ExpressionNode *Expr) {
  if (!Expr) {
    return nullptr;
  }

  switch (Expr->getKind()) {
  case NodeKind::LiteralExpression: {
    auto *Lit = static_cast<LiteralExpression *>(Expr);
    switch (Lit->LiteralKind) {
    case LiteralExpression::Integer:
      return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context),
                                    Lit->IntegerValue, true);
    case LiteralExpression::Float:
      // Real literals are f64 to match scalar Float and the runtime ABI.
      return llvm::ConstantFP::get(llvm::Type::getDoubleTy(Context),
                                   Lit->FloatValue);
    case LiteralExpression::Boolean:
      return llvm::ConstantInt::get(llvm::Type::getInt1Ty(Context),
                                    Lit->BooleanValue);
    case LiteralExpression::Character:
      return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context),
                                    static_cast<uint32_t>(Lit->CharacterValue));
    case LiteralExpression::String: {
      // Allocate string in global data section
      auto *StrTy = llvm::ArrayType::get(
          llvm::Type::getInt8Ty(Context), Lit->Value.size() + 1);
      llvm::Constant *Init = llvm::ConstantDataArray::getString(
          Context, Lit->Value, true);
      auto *GVar = new llvm::GlobalVariable(
          *Module, StrTy, true, llvm::GlobalValue::PrivateLinkage, Init,
          "str_lit");
      return Builder.CreateConstGEP1_32(StrTy, GVar, 0);
    }
    default:
      // Null literal needs runtime support (P5).
      return nullptr;
    }
  }
  case NodeKind::IdentifierExpression: {
    auto *Ident = static_cast<IdentifierExpression *>(Expr);
    auto It = NameMap.find(Ident->Name);
    if (It != NameMap.end()) {
      llvm::Value *Val = It->second;
      if (auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Val)) {
        return Builder.CreateLoad(Alloca->getAllocatedType(), Alloca,
                                  Ident->Name);
      }
      return Val;
    }
    // Parameterless runtime builtins used without parentheses
    // (e.g. Ada.Calendar.Clock, New_Line).
    {
      std::string Lower = ToLower(Ident->Name);
      if (Lower == "clock" || Lower == "new_line" || Lower == "flush") {
        llvm::Value *BuiltinValue = nullptr;
        if (GenerateRuntimeCall(Lower, {}, BuiltinValue))
          return BuiltinValue;
      }
    }
    std::cerr << "AdaCodeGen: identifier '" << Ident->Name
              << "' not found in symbol table\n";
    return nullptr;
  }
  case NodeKind::RecordFieldAccess: {
    llvm::Type *ElemType;
    llvm::Value *Ptr = GenerateLValue(Expr, ElemType);
    if (!Ptr) {
      return nullptr;
    }
    return Builder.CreateLoad(ElemType, Ptr,
                              static_cast<RecordFieldAccess *>(Expr)->FieldName);
  }
  case NodeKind::CallExpression:
    return GenerateCall(static_cast<CallExpression *>(Expr));
  case NodeKind::AttributeReference: {
    auto *Attr = static_cast<AttributeReference *>(Expr);
    // Only 'Result is supported in P3 (readable inside a Post aspect).
    if (Attr->AttributeName == "Result" && CurrentResult) {
      return Builder.CreateLoad(
          llvm::cast<llvm::AllocaInst>(CurrentResult)->getAllocatedType(),
          CurrentResult, "result");
    }
    // 'Tag attribute returns the tag global address for tagged objects
    if (Attr->AttributeName == "Tag" && Attr->Prefix) {
      // Get the tag slot (first field) of the tagged object
      llvm::Type *ElemType;
      llvm::Value *ObjPtr = GenerateLValue(Attr->Prefix.get(), ElemType);
      if (!ObjPtr) {
        return nullptr;
      }
      // The tag is stored in the first 8 bytes of the object
      llvm::Type *PtrTy = llvm::PointerType::get(Context, 0);
      llvm::Value *TagPtr = Builder.CreateBitCast(
          ObjPtr, llvm::PointerType::get(Context, 0), "tag_ptr");
      return Builder.CreateLoad(PtrTy, TagPtr, "tag");
    }
    return nullptr;
  }
  case NodeKind::UnaryExpression: {
    auto *Unary = static_cast<UnaryExpression *>(Expr);
    llvm::Value *Operand = GenerateExpression(Unary->Operand.get());
    if (!Operand) {
      return nullptr;
    }
    switch (Unary->Op) {
    case UnaryExpression::Negate:
      if (Operand->getType()->isIntegerTy()) {
        return Builder.CreateNSWNeg(Operand, "neg");
      }
      return Builder.CreateFNeg(Operand, "fneg");
    case UnaryExpression::Positive:
      return Operand;
    case UnaryExpression::Not:
      if (Operand->getType()->isIntegerTy(1)) {
        return Builder.CreateNot(Operand, "not");
      }
      return nullptr;
    }
    break;
  }
  case NodeKind::TypeConversionExpression: {
    auto *Conv = static_cast<TypeConversionExpression *>(Expr);
    llvm::Value *Operand = GenerateExpression(Conv->Operand.get());
    if (!Operand || !Conv->TargetType) {
      return Operand;
    }
    llvm::Type *TargetTy = GenerateType(Conv->TargetType.get());
    llvm::Type *OpTy = Operand->getType();
    if (OpTy == TargetTy) {
      return Operand;
    }
    if (OpTy->isIntegerTy() && TargetTy->isFloatTy()) {
      return Builder.CreateSIToFP(Operand, TargetTy, "conv");
    }
    if (OpTy->isFloatTy() && TargetTy->isIntegerTy()) {
      return Builder.CreateFPToSI(Operand, TargetTy, "conv");
    }
    return Operand;
  }
  case NodeKind::AllocationExpression: {
    auto *Alloc = static_cast<AllocationExpression *>(Expr);
    if (!Alloc->Type) {
      return nullptr;
    }
    llvm::Type *Type = GenerateType(Alloc->Type.get());
    if (!Type) {
      return nullptr;
    }
    // Get size of the type
    llvm::DataLayout DL = Module->getDataLayout();
    uint64_t TypeSize = DL.getTypeAllocSize(Type);
    // Call __ada_allocate(Size)
    auto *AllocFn = Module->getFunction("__ada_allocate");
    if (!AllocFn) {
      llvm::FunctionType *FnTy = llvm::FunctionType::get(
          llvm::PointerType::get(Context, 0),
          {llvm::Type::getInt64Ty(Context)}, false);
      AllocFn = llvm::Function::Create(
          FnTy, llvm::Function::ExternalLinkage,
          "__ada_allocate", *Module);
    }
    llvm::Value *SizeArg =
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), TypeSize);
    llvm::Value *AllocPtr = Builder.CreateCall(AllocFn, {SizeArg}, "alloc");
    // If there's an initializer, generate and store it
    if (Alloc->Initializer) {
      llvm::Value *InitVal = GenerateExpression(Alloc->Initializer.get());
      if (InitVal) {
        // Store the initializer to the allocated memory
        Builder.CreateStore(InitVal, AllocPtr);
      }
    }
    return AllocPtr;
  }
  case NodeKind::NullExpression:
    return nullptr;
  case NodeKind::BinaryExpression: {
    auto *Bin = static_cast<BinaryExpression *>(Expr);
    llvm::Value *Left = GenerateExpression(Bin->Left.get());
    llvm::Value *Right = GenerateExpression(Bin->Right.get());
    if (!Left || !Right) {
      return nullptr;
    }
    // Mixed integer/real operands (e.g. Clock > 0): promote the integer to
    // the floating-point type so the comparison/arithmetic is well-typed.
    if (Left->getType() != Right->getType()) {
      if (Left->getType()->isFloatingPointTy() &&
          Right->getType()->isIntegerTy())
        Right = Builder.CreateSIToFP(Right, Left->getType(), "conv");
      else if (Right->getType()->isFloatingPointTy() &&
               Left->getType()->isIntegerTy())
        Left = Builder.CreateSIToFP(Left, Right->getType(), "conv");
      else if (Left->getType()->isFloatingPointTy() &&
               Right->getType()->isFloatingPointTy()) {
        // Widen the narrower floating-point operand.
        if (Left->getType()->getPrimitiveSizeInBits() >
            Right->getType()->getPrimitiveSizeInBits())
          Right = Builder.CreateFPExt(Right, Left->getType(), "conv");
        else
          Left = Builder.CreateFPExt(Left, Right->getType(), "conv");
      }
    }
    bool IsInt = Left->getType()->isIntegerTy();
    bool IsBool = Left->getType()->isIntegerTy(1) &&
                  Right->getType()->isIntegerTy(1);

    switch (Bin->Op) {
    case BinaryExpression::Add:
      return IsInt ? Builder.CreateAdd(Left, Right, "add")
                   : Builder.CreateFAdd(Left, Right, "fadd");
    case BinaryExpression::Subtract:
      return IsInt ? Builder.CreateSub(Left, Right, "sub")
                   : Builder.CreateFSub(Left, Right, "fsub");
    case BinaryExpression::Multiply:
      return IsInt ? Builder.CreateMul(Left, Right, "mul")
                   : Builder.CreateFMul(Left, Right, "fmul");
    case BinaryExpression::Divide:
      return IsInt ? Builder.CreateSDiv(Left, Right, "sdiv")
                   : Builder.CreateFDiv(Left, Right, "fdiv");
    case BinaryExpression::Mod:
      if (IsInt) {
        return Builder.CreateSRem(Left, Right, "srem");
      }
      return nullptr;
    case BinaryExpression::And:
      return IsBool ? Builder.CreateAnd(Left, Right, "and") : nullptr;
    case BinaryExpression::Or:
      return IsBool ? Builder.CreateOr(Left, Right, "or") : nullptr;
    case BinaryExpression::Xor:
      return IsBool ? Builder.CreateXor(Left, Right, "xor") : nullptr;
    case BinaryExpression::Equal:
      return IsInt ? Builder.CreateICmpEQ(Left, Right, "cmp")
                   : Builder.CreateFCmpOEQ(Left, Right, "cmp");
    case BinaryExpression::NotEqual:
      return IsInt ? Builder.CreateICmpNE(Left, Right, "cmp")
                   : Builder.CreateFCmpONE(Left, Right, "cmp");
    case BinaryExpression::LessThan:
      return IsInt ? Builder.CreateICmpSLT(Left, Right, "cmp")
                   : Builder.CreateFCmpOLT(Left, Right, "cmp");
    case BinaryExpression::GreaterThan:
      return IsInt ? Builder.CreateICmpSGT(Left, Right, "cmp")
                   : Builder.CreateFCmpOGT(Left, Right, "cmp");
    case BinaryExpression::LessEqual:
      return IsInt ? Builder.CreateICmpSLE(Left, Right, "cmp")
                   : Builder.CreateFCmpOLE(Left, Right, "cmp");
    case BinaryExpression::GreaterEqual:
      return IsInt ? Builder.CreateICmpSGE(Left, Right, "cmp")
                   : Builder.CreateFCmpOGE(Left, Right, "cmp");
    default:
      // Power, Concat, In, Range: not yet supported (P3 follow-up).
      return nullptr;
    }
  }
  default:
    break;
  }

  return nullptr;
}

bool AdaCodeGen::GenerateTaggedAssignment(llvm::Value *Target,
                                          llvm::Value *Source,
                                          const std::string &TargetType,
                                          const std::string &SourceType) {
  auto TIt = TaggedLayouts.find(TargetType);
  auto SIt = TaggedLayouts.find(SourceType);
  if (TIt == TaggedLayouts.end() || SIt == TaggedLayouts.end()) {
    return false;
  }
  auto *TAlloca = llvm::dyn_cast<llvm::AllocaInst>(Target);
  auto *SAlloca = llvm::dyn_cast<llvm::AllocaInst>(Source);
  if (!TAlloca || !SAlloca) {
    return false;
  }
  llvm::Function *F = Builder.GetInsertBlock()->getParent();
  if (!F) {
    return false;
  }
  llvm::Type *PtrTy = llvm::PointerType::get(Context, 0);

  // Load the source tag once, before branching.
  llvm::Value *STag = Builder.CreateLoad(PtrTy, SAlloca, "tag");

  // Legal only if the source tag is the target type or a descendant.
  llvm::Value *Legal = nullptr;
  for (const std::string &Desc : TaggedDescendants(TargetType)) {
    auto GIt = TagGlobals.find(ToLower(Desc));
    if (GIt == TagGlobals.end()) {
      continue;
    }
    llvm::Value *Cmp = Builder.CreateICmpEQ(STag, GIt->second, "tag.cmp");
    Legal = Legal ? Builder.CreateOr(Legal, Cmp, "tag.legal") : Cmp;
  }
  if (!Legal) {
    return false;
  }

  auto *OkBB = llvm::BasicBlock::Create(Context, "assign.tag.ok", F);
  auto *ErrBB = llvm::BasicBlock::Create(Context, "assign.tag.err", F);
  Builder.CreateCondBr(Legal, OkBB, ErrBB);

  Builder.SetInsertPoint(ErrBB);
  llvm::FunctionCallee RaiseFn = Module->getOrInsertFunction(
      "__ada_raise",
      llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {PtrTy},
                              false));
  llvm::Constant *Msg = Builder.CreateGlobalString("Storage_Error");
  Builder.CreateCall(RaiseFn, {Msg});
  Builder.CreateUnreachable();

  Builder.SetInsertPoint(OkBB);
  // Copy the tag, then copy destination fields by name from the source.
  Builder.CreateStore(STag, TAlloca);
  auto &TL = TIt->second;
  for (size_t I = 0; I < TL.size(); I++) {
    size_t J = TL.size();
    for (size_t K = 0; K < SIt->second.size(); K++) {
      if (ToLower(SIt->second[K].first) == ToLower(TL[I].first)) {
        J = K;
        break;
      }
    }
    if (J >= SIt->second.size()) {
      continue;
    }
    auto *DP = Builder.CreateGEP(
        TaggedStructType(TargetType), TAlloca,
        {llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 0),
         llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), I + 1)});
    auto *SP = Builder.CreateGEP(
        TaggedStructType(SourceType), SAlloca,
        {llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 0),
         llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), J + 1)});
    llvm::Value *V = Builder.CreateLoad(SIt->second[J].second, SP);
    Builder.CreateStore(V, DP);
  }
  // Insertion point ends inside OkBB; the caller continues there.
  return true;
}

bool AdaCodeGen::GenerateTaggedDispatch(const CallExpression *Call,
                                        llvm::Function *StaticCallee,
                                        SubprogramDeclaration *StaticDecl,
                                        std::vector<llvm::Value *> &Args,
                                        llvm::Value *&Result) {
  if (!Call || !StaticCallee || Args.empty() || Call->Arguments.empty()) {
    return false;
  }
  // Only a by-reference first argument that is an object (an alloca) can
  // carry a tag; formal parameters are not dispatchable here (P3 limitation).
  auto *FirstArg = llvm::dyn_cast<llvm::AllocaInst>(Args[0]);
  if (!FirstArg) {
    return false;
  }
  auto *FTy = StaticCallee->getFunctionType();
  if (!FTy->getParamType(0)->isPointerTy()) {
    return false;
  }
  if (!StaticDecl || StaticDecl->FormalParameters.empty()) {
    return false;
  }
  auto *FirstFormal = StaticDecl->FormalParameters[0].get();
  if (!FirstFormal || !FirstFormal->Type ||
      FirstFormal->Type->getKind() != NodeKind::ScalarType) {
    return false;
  }
  std::string BaseKey =
      ToLower(static_cast<ScalarType *>(FirstFormal->Type.get())->Name);
  if (TaggedLayouts.find(BaseKey) == TaggedLayouts.end()) {
    return false;
  }

  llvm::Function *F = Builder.GetInsertBlock()->getParent();
  if (!F) {
    return false;
  }
  std::vector<std::string> Descs = TaggedDescendants(BaseKey);

  // Check blocks (one per descendant, base first), call blocks, and the
  // base fallback + result merge are all appended at the function tail.
  std::vector<llvm::BasicBlock *> CheckBBs;
  std::vector<llvm::BasicBlock *> CallBBs;
  for (const std::string &Desc : Descs) {
    CheckBBs.push_back(llvm::BasicBlock::Create(
        Context, "dispatch.check_" + ToLower(Desc), F));
    CallBBs.push_back(llvm::BasicBlock::Create(
        Context, "dispatch.call_" + ToLower(Desc), F));
  }
  auto *BaseBB = llvm::BasicBlock::Create(Context, "dispatch.base", F);
  auto *ResultBB = llvm::BasicBlock::Create(Context, "dispatch.result", F);

  llvm::BasicBlock *CallerBB = Builder.GetInsertBlock();
  for (size_t I = 0; I < Descs.size(); I++) {
    const std::string &Desc = Descs[I];
    llvm::BasicBlock *Next =
        (I + 1 < Descs.size()) ? CheckBBs[I + 1] : BaseBB;
    Builder.SetInsertPoint(CheckBBs[I]);
    llvm::Value *Tag = Builder.CreateLoad(llvm::PointerType::get(Context, 0),
                                          FirstArg, "dispatch.tag");
    auto GIt = TagGlobals.find(ToLower(Desc));
    if (GIt == TagGlobals.end()) {
      Builder.CreateBr(Next);
      continue;
    }
    llvm::Value *Eq = Builder.CreateICmpEQ(Tag, GIt->second, "dispatch.eq");
    Builder.CreateCondBr(Eq, CallBBs[I], Next);
  }

  std::vector<llvm::Value *> CallResults;
  for (size_t I = 0; I < Descs.size(); I++) {
    const std::string &Desc = Descs[I];
    llvm::Function *Callee = StaticCallee;
    if (Desc != BaseKey) {
      auto DIt = FunctionMap.find(
          ToLower(StaticDecl->Name) + "_" + ToLower(Desc));
      if (DIt != FunctionMap.end()) {
        Callee = DIt->second;
      }
    }
    Builder.SetInsertPoint(CallBBs[I]);
    llvm::Value *R = Builder.CreateCall(Callee, Args);
    CallResults.push_back(R);
    Builder.CreateBr(ResultBB);
  }

  Builder.SetInsertPoint(BaseBB);
  llvm::Value *BaseResult = Builder.CreateCall(StaticCallee, Args);
  Builder.CreateBr(ResultBB);

  // Rewire the caller to the first check block.
  Builder.SetInsertPoint(CallerBB);
  Builder.CreateBr(CheckBBs[0]);

  Builder.SetInsertPoint(ResultBB);
  if (FTy->getReturnType()->isVoidTy()) {
    Result = nullptr;
  } else {
    llvm::PHINode *Phi =
        llvm::PHINode::Create(FTy->getReturnType(), Descs.size() + 1,
                              "dispatch.result", ResultBB);
    for (size_t I = 0; I < Descs.size(); I++) {
      Phi->addIncoming(CallResults[I], CallBBs[I]);
    }
    Phi->addIncoming(BaseResult, BaseBB);
    Result = Phi;
  }
  return true;
}

void AdaCodeGen::GenerateTask(TaskDeclaration *Task, SemanticAnalyzer &Analyzer) {
  // Create task object on the heap (via __ada_create_task)
  // The task body function is __ada_task_<name>
  std::string TaskBodyFnName = "__ada_task_" + Task->Name;
  
  // Declare the task body function
  llvm::FunctionCallee TaskBodyFn = Module->getOrInsertFunction(
      TaskBodyFnName,
      llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {}, false));
  
  // Call __ada_create_task to spawn the pthread
  auto *PtrTy = llvm::PointerType::get(Context, 0);
  llvm::FunctionCallee CreateTaskFn = Module->getOrInsertFunction(
      "__ada_create_task",
      llvm::FunctionType::get(PtrTy, {PtrTy, PtrTy}, false));
  
  // Extract function pointer from FunctionCallee
  llvm::Value *TaskBodyFnPtr = TaskBodyFn.getCallee();
  llvm::Value *TaskObj = Builder.CreateCall(CreateTaskFn, {TaskBodyFnPtr, llvm::Constant::getNullValue(PtrTy)});
  
  // Register the task object in NameMap so entry calls can find it
  NameMap[Task->Name] = TaskObj;
  
  // If the task has an immediate body, generate it
  if (Task->Body) {
    CurrentTask = Task;
    GenerateTaskBody(Task->Body.get(), Analyzer);
    CurrentTask = nullptr;
  }
}

void AdaCodeGen::GenerateTaskBody(TaskBody *Body, SemanticAnalyzer &Analyzer) {
  // For now, treat task bodies like subprogram bodies
  // The pthread wrapper will be added later
  auto *Func = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getVoidTy(Context), {}, false),
      llvm::Function::ExternalLinkage, "__ada_task_" + Body->Name, *Module);
  
  // Generate the body
  auto *Entry = llvm::BasicBlock::Create(Context, "entry", Func);
  Builder.SetInsertPoint(Entry);
  
  // Generate declarations
  for (auto &D : Body->Declarations) {
    if (D) {
      GenerateDeclaration(D.get());
    }
  }
  
  // Generate body statements
  GenerateStatements(Body->BodyStatements);
  
  Builder.CreateRetVoid();
}
