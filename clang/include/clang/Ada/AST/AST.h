//===--- AST.h - Ada AST node definitions ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// All Ada AST node types in a single header to avoid circular dependencies.
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_ADA_AST_AST_H
#define CLANG_ADA_AST_AST_H

#include <map>
#include <string>
#include <vector>
#include <memory>

namespace clang {
namespace ada {

/// NodeKind identifies the type of each AST node.
enum class NodeKind {
  // Declarations
  PackageDeclaration,
  SubprogramDeclaration,
  ObjectDeclaration,
  TypeDeclaration,
  FormalParameter,

  // Generic declarations (P4)
  GenericSubprogramDeclaration,
  GenericPackageDeclaration,
  GenericInstantiation,
  TaskDeclaration,
  TaskBody,
  EntryDeclaration,

  // Types
  ScalarType,
  ArrayType,
  RecordType,
  AccessType,
  StringType,
  DerivedType,
  DiscriminatedType,

  // Expressions
  LiteralExpression,
  IdentifierExpression,
  BinaryExpression,
  UnaryExpression,
  CallExpression,
  TypeConversionExpression,
  AllocationExpression,
  AggregateExpression,
  ArrayElementAccess,
  RecordFieldAccess,
  AttributeReference,
  CaseExpression,
  ConditionalExpression,
  NullExpression,

  // Statements
  AssignmentStatement,
  IfStatement,
  LoopStatement,
  ForLoopStatement,
  WhileLoopStatement,
  CaseStatement,
  BlockStatement,
  ProcedureCallStatement,
  ReturnStatement,
  ExitStatement,
  NullStatement,
  RaiseStatement,
  TryStatement,
  AcceptStatement,
  DelayStatement,
  RequeueStatement,
  PragmaStatement,

  // Aspects/Pragmas
  AspectSpecification,
  PragmaDeclaration,
};

/// ASTNode is the base class for all Ada AST nodes.
class ASTNode {
public:
  explicit ASTNode(NodeKind Kind) : Kind(Kind) {}
  virtual ~ASTNode() = default;

  NodeKind getKind() const { return Kind; }
  virtual void dump() const = 0;

protected:
  NodeKind Kind;
};

// Forward declarations for node types
class DeclarationNode;
class TypeNode;
class ExpressionNode;
class StatementNode;
class AspectSpecification;

/// Declaration is the base class for all declaration nodes.
class DeclarationNode : public ASTNode {
public:
  explicit DeclarationNode(NodeKind Kind) : ASTNode(Kind) {}
  virtual ~DeclarationNode() = default;
};

/// TypeNode is the base class for all type nodes.
class TypeNode : public ASTNode {
public:
  explicit TypeNode(NodeKind Kind) : ASTNode(Kind), Frozen(false) {}
  virtual ~TypeNode() = default;

  /// True when the type has been frozen (definition complete)
  bool Frozen;
};

/// ExpressionNode is the base class for all expression nodes.
class ExpressionNode : public ASTNode {
public:
  explicit ExpressionNode(NodeKind Kind) : ASTNode(Kind) {}
  virtual ~ExpressionNode() = default;

  /// The inferred/declared type of this expression (set by semantic analysis)
  std::unique_ptr<TypeNode> Type;
};

/// StatementNode is the base class for all statement nodes.
class StatementNode : public ASTNode {
public:
  explicit StatementNode(NodeKind Kind) : ASTNode(Kind) {}
  virtual ~StatementNode() = default;
};

// Declarations

/// PackageDeclaration represents a package declaration (spec or body).
class PackageDeclaration : public DeclarationNode {
public:
  void dump() const override;
  PackageDeclaration() : DeclarationNode(NodeKind::PackageDeclaration),
                         IsBody(false), IsRenamed(false) {}
  std::string Name;
  bool IsBody;
  bool IsRenamed;
  std::string RenamedPackage;
  std::vector<std::unique_ptr<DeclarationNode>> Declarations;
  std::vector<std::unique_ptr<StatementNode>> BodyStatements;
  std::vector<std::unique_ptr<AspectSpecification>> Aspects;
};

/// Forward declaration for use in SubprogramDeclaration.
class FormalParameter;

/// Exception handler list shared by TryStatement and
/// subprogram-level exception handlers. First element: exception
/// name ("" or "others" for a catch-all).
using ExceptionHandlerList =
    std::vector<std::pair<std::string,
                          std::vector<std::unique_ptr<StatementNode>>>>;

/// SubprogramDeclaration represents a procedure or function declaration.
class SubprogramDeclaration : public DeclarationNode {
public:
  void dump() const override;
  SubprogramDeclaration()
      : DeclarationNode(NodeKind::SubprogramDeclaration), IsFunction(false),
        IsBody(false), IsAbstract(false), IsGeneric(false) {}

  std::string Name;
  bool IsFunction;
  bool IsBody;
  bool IsAbstract;
  bool IsGeneric;  // P4: true for generic subprogram specifications
  std::vector<std::unique_ptr<FormalParameter>> FormalParameters;
  std::unique_ptr<TypeNode> ReturnType;
  std::vector<std::unique_ptr<DeclarationNode>> Declarations;
  std::vector<std::unique_ptr<StatementNode>> BodyStatements;
  std::vector<std::unique_ptr<AspectSpecification>> Aspects;
  ExceptionHandlerList ExceptionHandlers;
};

/// GenericInstantiation represents a generic instantiation (is new).
class GenericInstantiation : public DeclarationNode {
public:
  void dump() const override;
  GenericInstantiation() : DeclarationNode(NodeKind::GenericInstantiation) {}

  std::string Name;  // Name of the instantiated entity
  std::string GenericName;  // Name of the generic
  // Formal parameter associations: name -> expression
  std::map<std::string, std::unique_ptr<ExpressionNode>> Actuals;
};

/// FormalParameter represents a subprogram formal parameter.
class FormalParameter : public DeclarationNode {
public:
  void dump() const override;
  FormalParameter() : DeclarationNode(NodeKind::FormalParameter),
                      IsInout(false) {}

  std::string Name;
  std::unique_ptr<TypeNode> Type;
  bool IsInout;
  std::unique_ptr<ExpressionNode> Default;
};

/// ObjectDeclaration represents a variable or constant declaration.
class ObjectDeclaration : public DeclarationNode {
public:
  void dump() const override;
  ObjectDeclaration() : DeclarationNode(NodeKind::ObjectDeclaration),
                        IsConstant(false) {}

  std::string Name;  // First name (for backwards compatibility)
  std::vector<std::string> Names;  // All declarator names (multiple: X, Y : T)
  std::unique_ptr<TypeNode> Type;
  std::unique_ptr<ExpressionNode> Initializer;
  bool IsConstant;
  std::vector<std::unique_ptr<AspectSpecification>> Aspects;
};

/// TypeDeclaration represents a type declaration.
class TypeDeclaration : public DeclarationNode {
public:
  void dump() const override;
  TypeDeclaration() : DeclarationNode(NodeKind::TypeDeclaration),
                      IsPrivate(false), IsTagged(false), IsAbstract(false) {}

  std::string Name;
  std::unique_ptr<TypeNode> Type;
  bool IsPrivate;
  bool IsTagged;    // `tagged` type (RM 3.9.2)
  bool IsAbstract;  // `abstract` tagged type (RM 3.9.2)
  std::string Parent;  // parent type name for `tagged new Parent` ("" if none)
  std::vector<std::unique_ptr<AspectSpecification>> Aspects;
};

// Types

/// ScalarType represents a scalar type (integer, float, character, enumeration).
class ScalarType : public TypeNode {
public:
  void dump() const override;
  ScalarType() : TypeNode(NodeKind::ScalarType) {}

  enum ScalarKind { Integer, Float, Character, Enumeration, Boolean };
  ScalarKind ScalarKind = Integer;

  std::string Name;
  std::unique_ptr<ExpressionNode> Low;
  std::unique_ptr<ExpressionNode> High;
  std::vector<std::string> Enumerators;
};

/// ArrayType represents an array type.
class ArrayType : public TypeNode {
public:
  void dump() const override;
  ArrayType() : TypeNode(NodeKind::ArrayType) {}

  std::vector<std::unique_ptr<ExpressionNode>> IndexExpressions;
  std::unique_ptr<TypeNode> ElementType;
};

/// RecordType represents a record type.
class RecordType : public TypeNode {
public:
  void dump() const override;
  RecordType() : TypeNode(NodeKind::RecordType) {}

  std::vector<std::unique_ptr<ObjectDeclaration>> Fields;
  std::vector<std::unique_ptr<AspectSpecification>> Aspects;
};

/// AccessType represents an access type (pointer).
class AccessType : public TypeNode {
public:
  void dump() const override;
  AccessType() : TypeNode(NodeKind::AccessType) {}

  std::unique_ptr<TypeNode> TargetType;
};

/// StringType represents a string type.
class StringType : public TypeNode {
public:
  void dump() const override;
  StringType() : TypeNode(NodeKind::StringType) {}

  std::unique_ptr<TypeNode> ElementType;
};

/// DerivedType represents a derived type.
class DerivedType : public TypeNode {
public:
  void dump() const override;
  DerivedType() : TypeNode(NodeKind::DerivedType) {}

  std::string ParentType;
  std::vector<std::unique_ptr<DeclarationNode>> Extensions;
};

/// DiscriminatedType represents a discriminated type.
class DiscriminatedType : public TypeNode {
public:
  void dump() const override;
  DiscriminatedType() : TypeNode(NodeKind::DiscriminatedType) {}

  std::vector<std::unique_ptr<FormalParameter>> Discriminants;
  std::vector<std::unique_ptr<DeclarationNode>> Fields;
};

// Forward declarations for task types
class TaskBody;

/// TaskDeclaration represents a task declaration (spec).
class TaskDeclaration : public DeclarationNode {
public:
  void dump() const override;
  TaskDeclaration() : DeclarationNode(NodeKind::TaskDeclaration), IsBody(false) {}

  std::string Name;
  bool IsBody;
  std::vector<std::unique_ptr<DeclarationNode>> Entries;
  std::unique_ptr<TaskBody> Body;
};

/// TaskBody represents a task body.
class TaskBody : public DeclarationNode {
public:
  void dump() const override;
  TaskBody() : DeclarationNode(NodeKind::TaskBody) {}

  std::string Name;
  std::vector<std::unique_ptr<DeclarationNode>> Declarations;
  std::vector<std::unique_ptr<StatementNode>> BodyStatements;
};

/// EntryDeclaration represents a task entry declaration.
class EntryDeclaration : public DeclarationNode {
public:
  void dump() const override;
  EntryDeclaration() : DeclarationNode(NodeKind::EntryDeclaration) {}

  std::string Name;
  std::vector<std::unique_ptr<FormalParameter>> FormalParameters;
};

// Expressions

/// LiteralExpression represents a literal value.
class LiteralExpression : public ExpressionNode {
public:
  void dump() const override;
  LiteralExpression() : ExpressionNode(NodeKind::LiteralExpression) {}

  enum LiteralKind { Integer, Float, String, Character, Boolean, Null };
  LiteralKind LiteralKind = Integer;

  std::string Value;
  int64_t IntegerValue = 0;
  double FloatValue = 0.0;
  char CharacterValue = 0;
  bool BooleanValue = false;
};

/// IdentifierExpression represents an identifier.
class IdentifierExpression : public ExpressionNode {
public:
  void dump() const override;
  IdentifierExpression() : ExpressionNode(NodeKind::IdentifierExpression) {}

  std::string Name;
  std::vector<std::string> Qualifiers;
};

/// BinaryExpression represents a binary operation.
class BinaryExpression : public ExpressionNode {
public:
  void dump() const override;
  BinaryExpression() : ExpressionNode(NodeKind::BinaryExpression) {}

  enum BinaryOp {
    Add, Subtract, Multiply, Divide, Mod, Rem, Power,
    And, Or, Xor, Nand, Nor,
    Equal, NotEqual, LessThan, GreaterThan, LessEqual, GreaterEqual,
    Concat, In, Range
  };
  BinaryOp Op = Add;

  std::unique_ptr<ExpressionNode> Left;
  std::unique_ptr<ExpressionNode> Right;
};

/// UnaryExpression represents a unary operation.
class UnaryExpression : public ExpressionNode {
public:
  void dump() const override;
  UnaryExpression() : ExpressionNode(NodeKind::UnaryExpression) {}

  enum UnaryOp { Negate, Positive, Not };
  UnaryOp Op = Negate;

  std::unique_ptr<ExpressionNode> Operand;
};

/// CallExpression represents a function or procedure call.
class CallExpression : public ExpressionNode {
public:
  void dump() const override;
  CallExpression() : ExpressionNode(NodeKind::CallExpression) {}

  std::unique_ptr<ExpressionNode> Function;
  std::vector<std::unique_ptr<ExpressionNode>> Arguments;
  std::vector<std::string> NamedArguments;
  SubprogramDeclaration *Resolved = nullptr;  // set by semantics
};

/// TypeConversionExpression represents a type conversion.
class TypeConversionExpression : public ExpressionNode {
public:
  void dump() const override;
  TypeConversionExpression()
      : ExpressionNode(NodeKind::TypeConversionExpression) {}

  std::unique_ptr<TypeNode> TargetType;
  std::unique_ptr<ExpressionNode> Operand;
};

/// AllocationExpression represents dynamic memory allocation (new).
class AllocationExpression : public ExpressionNode {
public:
  void dump() const override;
  AllocationExpression() : ExpressionNode(NodeKind::AllocationExpression) {}

  std::unique_ptr<TypeNode> Type;
  std::unique_ptr<ExpressionNode> Initializer;  // null if uninitialized
};

/// AggregateExpression represents an aggregate value.
class AggregateExpression : public ExpressionNode {
public:
  void dump() const override;
  AggregateExpression() : ExpressionNode(NodeKind::AggregateExpression) {}

  enum AggregateKind { Positional, Named, Array, Record };
  AggregateKind AggregateKind = Positional;

  std::vector<std::unique_ptr<ExpressionNode>> Elements;
  std::vector<std::string> Names;
};

/// ArrayElementAccess represents array element access.
class ArrayElementAccess : public ExpressionNode {
public:
  void dump() const override;
  ArrayElementAccess() : ExpressionNode(NodeKind::ArrayElementAccess) {}

  std::unique_ptr<ExpressionNode> Array;
  std::unique_ptr<ExpressionNode> Index;
};

/// RecordFieldAccess represents record field access.
class RecordFieldAccess : public ExpressionNode {
public:
  void dump() const override;
  RecordFieldAccess() : ExpressionNode(NodeKind::RecordFieldAccess) {}

  std::unique_ptr<ExpressionNode> Record;
  std::string FieldName;
};

/// AttributeReference represents an attribute reference.
class AttributeReference : public ExpressionNode {
public:
  void dump() const override;
  AttributeReference() : ExpressionNode(NodeKind::AttributeReference) {}

  std::unique_ptr<ExpressionNode> Prefix;
  std::string AttributeName;
};

/// CaseExpression represents a case expression.
class CaseExpression : public ExpressionNode {
public:
  void dump() const override;
  CaseExpression() : ExpressionNode(NodeKind::CaseExpression) {}

  std::unique_ptr<ExpressionNode> Discriminant;
  std::vector<std::unique_ptr<ExpressionNode>> Choices;
  std::vector<std::unique_ptr<ExpressionNode>> Results;
};

/// ConditionalExpression represents a conditional expression.
class ConditionalExpression : public ExpressionNode {
public:
  void dump() const override;
  ConditionalExpression() : ExpressionNode(NodeKind::ConditionalExpression) {}

  std::unique_ptr<ExpressionNode> Condition;
  std::unique_ptr<ExpressionNode> ThenExpr;
  std::unique_ptr<ExpressionNode> ElseExpr;
};

/// NullExpression represents a null value.
class NullExpression : public ExpressionNode {
public:
  void dump() const override;
  NullExpression() : ExpressionNode(NodeKind::NullExpression) {}
};

// Statements

/// AssignmentStatement represents an assignment.
class AssignmentStatement : public StatementNode {
public:
  void dump() const override;
  AssignmentStatement() : StatementNode(NodeKind::AssignmentStatement) {}

  std::unique_ptr<ExpressionNode> Target;
  std::unique_ptr<ExpressionNode> Value;
};

/// IfStatement represents an if statement.
class IfStatement : public StatementNode {
public:
  void dump() const override;
  IfStatement() : StatementNode(NodeKind::IfStatement) {}

  std::unique_ptr<ExpressionNode> Condition;
  std::vector<std::unique_ptr<StatementNode>> ThenStatements;
  std::vector<std::pair<std::unique_ptr<ExpressionNode>,
                        std::vector<std::unique_ptr<StatementNode>>>>
      Elsifs;
  std::vector<std::unique_ptr<StatementNode>> ElseStatements;
};

/// LoopStatement represents a loop statement.
class LoopStatement : public StatementNode {
public:
  void dump() const override;
  LoopStatement() : StatementNode(NodeKind::LoopStatement) {}

  std::vector<std::unique_ptr<StatementNode>> Body;
};

/// ForLoopStatement represents a for loop.
class ForLoopStatement : public StatementNode {
public:
  void dump() const override;
  ForLoopStatement() : StatementNode(NodeKind::ForLoopStatement) {}

  std::string Variable;
  std::unique_ptr<ExpressionNode> Low;
  std::unique_ptr<ExpressionNode> High;
  std::vector<std::unique_ptr<StatementNode>> Body;
  bool IsReverse = false;
};

/// WhileLoopStatement represents a while loop.
class WhileLoopStatement : public StatementNode {
public:
  void dump() const override;
  WhileLoopStatement() : StatementNode(NodeKind::WhileLoopStatement) {}

  std::unique_ptr<ExpressionNode> Condition;
  std::vector<std::unique_ptr<StatementNode>> Body;
};

/// CaseStatement represents a case statement.
class CaseStatement : public StatementNode {
public:
  void dump() const override;
  CaseStatement() : StatementNode(NodeKind::CaseStatement) {}

  std::unique_ptr<ExpressionNode> Discriminant;
  std::vector<std::pair<std::vector<std::unique_ptr<ExpressionNode>>,
                        std::vector<std::unique_ptr<StatementNode>>>>
      Alternatives;
};

/// BlockStatement represents a block.
class BlockStatement : public StatementNode {
public:
  void dump() const override;
  BlockStatement() : StatementNode(NodeKind::BlockStatement) {}

  std::string Label;
  std::vector<std::unique_ptr<DeclarationNode>> Declarations;
  std::vector<std::unique_ptr<StatementNode>> Statements;
};

/// ProcedureCallStatement represents a procedure call.
class ProcedureCallStatement : public StatementNode {
public:
  void dump() const override;
  ProcedureCallStatement() : StatementNode(NodeKind::ProcedureCallStatement) {}

  std::unique_ptr<ExpressionNode> Procedure;
  std::vector<std::unique_ptr<ExpressionNode>> Arguments;
  std::vector<std::string> NamedArguments;
};

/// ReturnStatement represents a return statement.
class ReturnStatement : public StatementNode {
public:
  void dump() const override;
  ReturnStatement() : StatementNode(NodeKind::ReturnStatement) {}

  std::unique_ptr<ExpressionNode> Value;
};

/// ExitStatement represents an exit statement.
class ExitStatement : public StatementNode {
public:
  void dump() const override;
  ExitStatement() : StatementNode(NodeKind::ExitStatement) {}

  std::string LoopLabel;
  std::unique_ptr<ExpressionNode> Condition;
};

/// NullStatement represents a null statement.
class NullStatement : public StatementNode {
public:
  void dump() const override;
  NullStatement() : StatementNode(NodeKind::NullStatement) {}
};

/// RaiseStatement represents a raise statement.
class RaiseStatement : public StatementNode {
public:
  void dump() const override;
  RaiseStatement() : StatementNode(NodeKind::RaiseStatement) {}

  std::unique_ptr<ExpressionNode> Exception;
};

/// TryStatement represents a try/except block (RM 9.5.2).
class TryStatement : public StatementNode {
public:
  void dump() const override;
  TryStatement() : StatementNode(NodeKind::TryStatement) {}

  std::vector<std::unique_ptr<StatementNode>> Body;
  ExceptionHandlerList Handlers;
};

/// AcceptStatement represents an accept statement (RM 9.5.1).
class AcceptStatement : public StatementNode {
public:
  void dump() const override;
  AcceptStatement() : StatementNode(NodeKind::AcceptStatement) {}

  std::string EntryName;
  std::vector<std::unique_ptr<FormalParameter>> FormalParameters;
  std::vector<std::unique_ptr<StatementNode>> Body;
};

/// DelayStatement represents a delay statement (RM 9.6).
class DelayStatement : public StatementNode {
public:
  void dump() const override;
  DelayStatement() : StatementNode(NodeKind::DelayStatement) {}

  std::unique_ptr<ExpressionNode> Duration;
};

/// RequeueStatement represents a requeue statement (RM 9.6.2).
class RequeueStatement : public StatementNode {
public:
  void dump() const override;
  RequeueStatement() : StatementNode(NodeKind::RequeueStatement) {}

  std::string EntryName;
  bool IsOthers;
};

/// PragmaStatement represents a pragma used as a statement (RM 5.1).
/// Pragmas are allowed anywhere a statement is; most are no-ops at runtime
/// but semantic analysis still validates the pragma name.
class PragmaStatement : public StatementNode {
public:
  void dump() const override;
  PragmaStatement() : StatementNode(NodeKind::PragmaStatement) {}

  std::string PragmaName;
  std::vector<std::unique_ptr<ExpressionNode>> Arguments;
};

// Aspects/Pragmas

/// AspectSpecification represents an aspect specification.
class AspectSpecification : public ASTNode {
public:
  void dump() const override;
  AspectSpecification() : ASTNode(NodeKind::AspectSpecification) {}

  std::string AspectName;
  std::unique_ptr<ExpressionNode> AspectValue;
};

/// PragmaDeclaration represents a pragma.
class PragmaDeclaration : public DeclarationNode {
public:
  void dump() const override;
  PragmaDeclaration() : DeclarationNode(NodeKind::PragmaDeclaration) {}

  std::string PragmaName;
  std::vector<std::unique_ptr<ExpressionNode>> Arguments;
};

} // namespace ada
} // namespace clang

#endif // CLANG_ADA_AST_AST_H