//===-- llvm/Instruction.h - Instruction class definition -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the Instruction class, which is the
// base class for all of the LLVM instructions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_INSTRUCTION_H
#define LLVM_INSTRUCTION_H

#include "llvm/User.h"

namespace llvm {

struct AssemblyAnnotationWriter;
class BinaryOperator;

template<typename ValueSubClass, typename ItemParentClass>
  class SymbolTableListTraits;

class Instruction : public User {
  void operator=(const Instruction &);     // Do not implement
  Instruction(const Instruction &);        // Do not implement

  BasicBlock *Parent;
  Instruction *Prev, *Next; // Next and Prev links for our intrusive linked list

  void setNext(Instruction *N) { Next = N; }
  void setPrev(Instruction *N) { Prev = N; }

  friend class SymbolTableListTraits<Instruction, BasicBlock>;
  void setParent(BasicBlock *P);
protected:
  Instruction(const Type *Ty, unsigned iType, Use *Ops, unsigned NumOps,
              Instruction *InsertBefore = 0);
  Instruction(const Type *Ty, unsigned iType, Use *Ops, unsigned NumOps,
              BasicBlock *InsertAtEnd);
public:
  // Out of line virtual method, so the vtable, etc has a home.
  ~Instruction();
  
  /// mayWriteToMemory - Return true if this instruction may modify memory.
  ///
  bool mayWriteToMemory() const;

  /// mayReadFromMemory - Return true if this instruction may read memory.
  ///
  bool mayReadFromMemory() const;
  
  /// clone() - Create a copy of 'this' instruction that is identical in all
  /// ways except the following:
  ///   * The instruction has no parent
  ///   * The instruction has no name
  ///
  virtual Instruction *clone() const = 0;

  /// isIdenticalTo - Return true if the specified instruction is exactly
  /// identical to the current one.  This means that all operands match and any
  /// extra information (e.g. load is volatile) agree.
  bool isIdenticalTo(Instruction *I) const;

  /// This function determines if the specified instruction executes the same
  /// operation as the current one. This means that the opcodes, type, operand
  /// types and any other factors affecting the operation must be the same. This
  /// is similar to isIdenticalTo except the operands themselves don't have to
  /// be identical.
  /// @returns true if the specified instruction is the same operation as
  /// the current one.
  /// @brief Determine if one instruction is the same operation as another.
  bool isSameOperationAs(Instruction *I) const;

  /// isUsedOutsideOfBlock - Return true if there are any uses of this
  /// instruction in blocks other than the specified block.  Note that PHI nodes
  /// are considered to evaluate their operands in the corresponding predecessor
  /// block.
  bool isUsedOutsideOfBlock(const BasicBlock *BB) const;
  
  
  /// use_back - Specialize the methods defined in Value, as we know that an
  /// instruction can only be used by other instructions.
  Instruction       *use_back()       { return cast<Instruction>(*use_begin());}
  const Instruction *use_back() const { return cast<Instruction>(*use_begin());}
  
  // Accessor methods...
  //
  inline const BasicBlock *getParent() const { return Parent; }
  inline       BasicBlock *getParent()       { return Parent; }

  /// removeFromParent - This method unlinks 'this' from the containing basic
  /// block, but does not delete it.
  ///
  void removeFromParent();

  /// eraseFromParent - This method unlinks 'this' from the containing basic
  /// block and deletes it.
  ///
  void eraseFromParent();

  /// moveBefore - Unlink this instruction from its current basic block and
  /// insert it into the basic block that MovePos lives in, right before
  /// MovePos.
  void moveBefore(Instruction *MovePos);

  // ---------------------------------------------------------------------------
  /// Subclass classification... getOpcode() returns a member of
  /// one of the enums that is coming soon (down below)...
  ///
  unsigned getOpcode() const { return getValueID() - InstructionVal; }
  const char *getOpcodeName() const { return getOpcodeName(getOpcode()); }
  bool isTerminator() const { return isTerminator(getOpcode()); }
  bool isBinaryOp() const { return isBinaryOp(getOpcode()); }
  bool isShift() { return isShift(getOpcode()); }
  bool isCast() const { return isCast(getOpcode()); }
  
  
  
  static const char* getOpcodeName(unsigned OpCode);

  static inline bool isTerminator(unsigned OpCode) {
    return OpCode >= TermOpsBegin && OpCode < TermOpsEnd;
  }

  static inline bool isBinaryOp(unsigned Opcode) {
    return Opcode >= BinaryOpsBegin && Opcode < BinaryOpsEnd;
  }

  /// @brief Determine if the Opcode is one of the shift instructions.
  static inline bool isShift(unsigned Opcode) {
    return Opcode >= Shl && Opcode <= AShr;
  }

  /// isLogicalShift - Return true if this is a logical shift left or a logical
  /// shift right.
  inline bool isLogicalShift() const {
    return getOpcode() == Shl || getOpcode() == LShr;
  }

  /// isLogicalShift - Return true if this is a logical shift left or a logical
  /// shift right.
  inline bool isArithmeticShift() const {
    return getOpcode() == AShr;
  }

  /// @brief Determine if the OpCode is one of the CastInst instructions.
  static inline bool isCast(unsigned OpCode) {
    return OpCode >= CastOpsBegin && OpCode < CastOpsEnd;
  }

  /// isAssociative - Return true if the instruction is associative:
  ///
  ///   Associative operators satisfy:  x op (y op z) === (x op y) op z
  ///
  /// In LLVM, the Add, Mul, And, Or, and Xor operators are associative, when
  /// not applied to floating point types.
  ///
  bool isAssociative() const { return isAssociative(getOpcode(), getType()); }
  static bool isAssociative(unsigned op, const Type *Ty);

  /// isCommutative - Return true if the instruction is commutative:
  ///
  ///   Commutative operators satisfy: (x op y) === (y op x)
  ///
  /// In LLVM, these are the associative operators, plus SetEQ and SetNE, when
  /// applied to any type.
  ///
  bool isCommutative() const { return isCommutative(getOpcode()); }
  static bool isCommutative(unsigned op);

  /// isTrappingInstruction - Return true if the instruction may trap.
  ///
  bool isTrapping() const {
    return isTrapping(getOpcode());
  }
  static bool isTrapping(unsigned op);

  virtual void print(std::ostream &OS) const { print(OS, 0); }
  void print(std::ostream *OS) const { if (OS) print(*OS); }
  void print(std::ostream &OS, AssemblyAnnotationWriter *AAW) const;

  /// Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const Instruction *) { return true; }
  static inline bool classof(const Value *V) {
    return V->getValueID() >= Value::InstructionVal;
  }

  //----------------------------------------------------------------------
  // Exported enumerations...
  //
  enum TermOps {       // These terminate basic blocks
#define  FIRST_TERM_INST(N)             TermOpsBegin = N,
#define HANDLE_TERM_INST(N, OPC, CLASS) OPC = N,
#define   LAST_TERM_INST(N)             TermOpsEnd = N+1
#include "llvm/Instruction.def"
  };

  enum BinaryOps {
#define  FIRST_BINARY_INST(N)             BinaryOpsBegin = N,
#define HANDLE_BINARY_INST(N, OPC, CLASS) OPC = N,
#define   LAST_BINARY_INST(N)             BinaryOpsEnd = N+1
#include "llvm/Instruction.def"
  };

  enum MemoryOps {
#define  FIRST_MEMORY_INST(N)             MemoryOpsBegin = N,
#define HANDLE_MEMORY_INST(N, OPC, CLASS) OPC = N,
#define   LAST_MEMORY_INST(N)             MemoryOpsEnd = N+1
#include "llvm/Instruction.def"
  };

  enum CastOps {
#define  FIRST_CAST_INST(N)             CastOpsBegin = N,
#define HANDLE_CAST_INST(N, OPC, CLASS) OPC = N,
#define   LAST_CAST_INST(N)             CastOpsEnd = N+1
#include "llvm/Instruction.def"
  };

  enum OtherOps {
#define  FIRST_OTHER_INST(N)             OtherOpsBegin = N,
#define HANDLE_OTHER_INST(N, OPC, CLASS) OPC = N,
#define   LAST_OTHER_INST(N)             OtherOpsEnd = N+1
#include "llvm/Instruction.def"
  };
  
private:
  // getNext/Prev - Return the next or previous instruction in the list.  The
  // last node in the list is a terminator instruction.
  Instruction *getNext()             { return Next; }
  const Instruction *getNext() const { return Next; }
  Instruction *getPrev()             { return Prev; }
  const Instruction *getPrev() const { return Prev; }
};

} // End llvm namespace

#endif
