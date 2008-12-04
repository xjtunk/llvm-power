/*
This pass shows a proof of concept implementation of dynamic optimization.
It first instruments all functions to print "<name> is cold" and increment
an execution count each time it's run. Upon reaching a threshold, it writes
a message to a queue that another thread created by the execution manager
reads and runs additional passes on the function.

*/


#include "llvm/Transforms/Instrumentation.h"
#include <set>
#include <queue>
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Constants.h"
#include "llvm/Module.h"
#include "llvm/ValueSymbolTable.h"
#include "llvm/ModuleProvider.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/MutexGuard.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "../lib/ExecutionEngine/JIT/JIT.h"
#include "llvm/DerivedTypes.h"

//// Brooks
//// including loopinfo.h to use the iterator for loops
#include "llvm/CodeGen/LoopInfo.h"

using namespace std;
using namespace llvm;


queue <string> *g_hot_messages;
sys :: Mutex *g_jit_lock;

void InsertMessage(Function *F)
{
  printf("Back into the runtime: %s\n", F->getName().c_str());
///  MutexGuard locked(*g_jit_lock);
///  g_hot_messages->push(m);
}

void CreatePrintfDeclaration(Module &)
{
}

#if 0
namespace {
  class VISIBILITY_HIDDEN EdgeProfiler : public ModulePass {
    bool runOnModule(Module &M);
  public:
    static char ID; // Pass identification, replacement for typeid
    EdgeProfiler() : ModulePass((intptr_t)&ID) {}
  };

  char EdgeProfiler::ID = 0;
  RegisterPass<EdgeProfiler> X("insert-edge-profiling",
                               "Insert instrumentation for edge profiling");
}
#endif

class ReturnToJITPass: public ModulePass
{
public:
    ReturnToJITPass() : ModulePass((intptr_t) & ID)
    {
    }
    bool runOnModule(Module &M)
    {
      return true;
      CreatePrintfDeclaration(M);

       Constant *zeros[] =
       {
          ConstantInt::getNullValue(Type :: Int32Ty),
          ConstantInt::getNullValue(Type :: Int32Ty)
       };
       Constant *hot_threshold = ConstantInt :: get(Type :: Int32Ty, 20),
                *printf_function = M.getFunction("printf");
       assert(printf_function!=0);

      for (Module::iterator f = M.begin(); f != M.end(); ++f)
      {
        if (f->isDeclaration())
          continue;

        BasicBlock *old_entry = f->begin(),
                   *new_entry = BasicBlock :: Create("", f, old_entry),
                   *hot_block = BasicBlock :: Create("", f, old_entry);

		//// Brooks
		//// Call Ahmad's routine at the beginning of each function
		AddTrampoline(old_entry, 1/*threshold*/);

		//// Brooks
		//// Also, call Ahmad's routine at the beginning of each loop in the function
		AnalysisUsage AU;
		AU.addRequired<LoopInfo>(); // where does AU come from?
		LoopInfo * LI; 
		LI = &getAnalysis<LoopInfo>(); // does this work? (if not, try LoopInfo::getAnalysis
		for (LoopInfo::iterator lii = LI->begin, liend = LI->end(); lii != liend; ++lii) {
			Loop * currloop = *lii;
			AddTrampoline(currloop->getHeader(), 1/*threshold*/);
		}

      }
      //cout << M;
      return true;
  }
  void AddTrampoline(BasicBlock * BB, int threshold) {
	  //hi
  }
  static char ID;
};
char ReturnToJITPass::ID = 0;
RegisterPass<ReturnToJITPass>
    X("dopt", "dynamic optimization example");


// right now, just creates a copy that prints "optimized" at the beginning
void CreateOptimizedFunction(Function *f)
{
   Instruction *first = f->front().getFirstNonPHI();

   Constant* constStr = ConstantArray::get(f->getName() + "_optimized\n");
   GlobalVariable *global =
      new GlobalVariable(constStr->getType(), true, GlobalValue::InternalLinkage, constStr, "message", f->getParent());
    Constant *zeros[] =
    {
       ConstantInt::getNullValue(Type :: Int32Ty),
       ConstantInt::getNullValue(Type :: Int32Ty)
    };
   Constant *constPtr = ConstantExpr::getGetElementPtr(global, zeros, 2);
   Constant *printf_function = f->getParent()->getFunction("printf");
   assert(printf_function!=0);
   CallInst::Create(printf_function, constPtr, "", first);
}

void DeleteInstrumentation(Function *f)   // need to make this a pass
{
  BasicBlock &entry = f->getEntryBlock();
  BranchInst *branch = (BranchInst *)&entry.back();
  BasicBlock &hot_block = *branch->getSuccessor(0),
         &original_entry = *branch->getSuccessor(1);

  original_entry.removePredecessor(&hot_block);
  original_entry.removePredecessor(&entry);

  entry.dropAllReferences();
  hot_block.dropAllReferences();
  entry.eraseFromParent();
  hot_block.eraseFromParent();
}

void OptimizeFunction(JIT *jit, Function *f)
{
  DeleteInstrumentation(f);
  
  cout << "AFTER REMOVING INSTRUMENTATION===============\n " << *f;
#if 0
  CreateOptimizedFunction(f);
#endif


#if 1
  jit->recompileAndRelinkFunction(f);
#else
  // custom code to do patching
  jit->runJITOnFunction(replacement);

  uint8_t *code1 = (uint8_t *)jit->getPointerToGlobalIfAvailable(f);
  uint8_t *code2 = (uint8_t *)jit->getPointerToGlobalIfAvailable(replacement);
  cout << "old code at " << (void *)code1 << ", replacement at " << (void *)code2 << endl;

  // create branch to new code (dangerous - not atomic)
  code1[0] = 0xe9;
  *(int32_t *)&code1[1] = code2 - (code1 + 5);
#endif
}


ModulePass *llvm::createReturnToJITPass()
{
  return new ReturnToJITPass();
///  return NULL;
}


