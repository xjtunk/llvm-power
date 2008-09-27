//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "hello"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Streams.h"
#include "llvm/ADT/Statistic.h"
#include  "llvm/Instruction.h"

// Ahmad
#define endl "\n"

using namespace llvm;

STATISTIC(HelloCounter, "Counts number of functions greeted");

namespace {
  // Hello - The first implementation, without getAnalysisUsage.
  struct Hello : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    Hello() : FunctionPass((intptr_t)&ID) {}

    virtual bool runOnFunction(Function &F) {
        // Instruction.h contains the definition for instructions. We can figure out the type
        // by doing getOpCode and inspecting it.
        // Integer ALU instructions: 
        // ADD, SUB, MUL, SDIV, UDIV, SREM, UREM,
        // Floating point ALU instructions:
        // FADD, FSUB, FMUL, FDIV, FREM,
        // Actual instructions are in instruction.def
      int bbcounter=0;
      int numMemory=0, numINT=0, numFP=0;
      unsigned opcode;
      HelloCounter++;
      std::string fname = F.getName();
      EscapeString(fname);
      cerr << "Hello: " << fname << "\n";

      // Ahmad modified here...
      cerr << "Ahmad here. Begin looking at instructions and dumping their opcodes" << endl;
      for( Function::iterator i=F.begin(); i!=F.end() ; i++ )
      {
          cerr << "Basic block: " << bbcounter << endl;
          for( BasicBlock::iterator j=i->begin() ; j!=i->end() ; j++ )
          {
              cerr << *j ;
              
              // This will print out the instruction's opcode.
///              cerr << j->getOpcodeName() << endl;
              opcode = j->getOpcode();
///              if(opcode>=Instruction::ADD && opcode <Instruction::UREM)
///                  numINT++;
///              else if(opcode>=Instruction::FADD && opcode<Instruction::FREM)
///                  numFP++;
              if(opcode==Instruction::FDiv)
                  numFP++;
          }

          cerr << "Done with inner loop" << endl;
          
///          cerr << *i << endl;
          bbcounter++;
          if(numFP==0)
          {
              cerr << "TURN THE FP DIV OFF!" << endl;
          }
      }
      

      return false;
    }
  };

  char Hello::ID = 0;
  RegisterPass<Hello> X("hello", "Hello World Pass");

  // Hello2 - The second implementation with getAnalysisUsage implemented.
  struct Hello2 : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    Hello2() : FunctionPass((intptr_t)&ID) {}

    virtual bool runOnFunction(Function &F) {
      HelloCounter++;
      std::string fname = F.getName();
      EscapeString(fname);
      cerr << "Hello: " << fname << "\n";
      return false;
    }

    // We don't modify the program, so we preserve all analyses
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
    };
  };
  char Hello2::ID = 0;
  RegisterPass<Hello2> Y("hello2",
                        "Hello World Pass (with getAnalysisUsage implemented)");
}
