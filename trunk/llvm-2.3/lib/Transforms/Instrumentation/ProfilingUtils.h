//===- ProfilingUtils.h - Helper functions shared by profilers --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines a few helper functions which are used by profile
// instrumentation code to instrument the code.  This allows the profiler pass
// to worry about *what* to insert, and these functions take care of *how* to do
// it.
//
//===----------------------------------------------------------------------===//

#ifndef PROFILINGUTILS_H
#define PROFILINGUTILS_H

// ahmad
#include  <vector>
#include  <map>
#include  "llvm/Value.h"
#include  "llvm/Function.h"

extern std::map<llvm::Function*, std::vector<llvm::Value*> > InstructionMap;
extern std::map<llvm::Function*, std::vector<llvm::BasicBlock*> > BasicBlockMap;

namespace llvm {
  class Function;
  class GlobalValue;
  class BasicBlock;

  void InsertProfilingInitCall(Function *MainFn, const char *FnName,
                               GlobalValue *Arr = 0);
  // ahmad added
  void IncrementCounterInBlock(BasicBlock *BB, unsigned CounterNum,
                               GlobalValue *CounterArray, 
                               std::vector<Value*> * InstructionArray=NULL);
  // brooks added
  void RemoveCountersInFunction(Function *F);
}

#endif
