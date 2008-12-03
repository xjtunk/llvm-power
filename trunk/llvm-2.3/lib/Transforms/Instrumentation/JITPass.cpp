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

        Constant *one = ConstantInt :: get(Type :: Int32Ty, 1),
              *zero = ConstantInt :: get(Type :: Int32Ty, 0);

        Value *counter =
                        new GlobalVariable(zero->getType(), false, GlobalValue::InternalLinkage, hot_threshold, f->getName() + "_execution_count", &M);


        Value *result = BinaryOperator::createSub(new LoadInst(counter, "", new_entry), one, "", new_entry);
        new StoreInst(result, counter, new_entry);

        Constant *message = ConstantArray :: get(f->getName() + " is cold\n");
        GlobalVariable *message_global = new GlobalVariable(message->getType(), true, GlobalValue::InternalLinkage, message, "", &M);
        message = ConstantExpr::getGetElementPtr(message_global, zeros, 2);
        CallInst::Create(printf_function, message, "", new_entry);

        BranchInst :: Create(hot_block, old_entry, new ICmpInst(ICmpInst::ICMP_EQ, result, zero, "", new_entry), new_entry);

        //-----------------------------------hot block---------------------------------------
        // ahmad added
        vector <const Type *> parameter_types(1);
        SmallVector<Value*, 8> Args;
        Function * fptr=f;
        Constant *C = 0;
        if (sizeof(void*) == 4)
        {
          C = ConstantInt::get(Type::Int32Ty, (int)(intptr_t)(fptr));
          parameter_types[0] = PointerType :: get(Type :: Int32Ty, 0);
        }
        else
        {
          C = ConstantInt::get(Type::Int64Ty, (intptr_t)(fptr));
          parameter_types[0] = PointerType :: get(Type :: Int64Ty, 0);
        }
        C = ConstantExpr::getIntToPtr(C, parameter_types[0]);  // Cast the integer to pointer
        Args.push_back(C);

///        parameter_types[0] = PointerType :: get(Type :: Int8Ty, 0);

        Constant *fname = ConstantArray :: get(f->getName());
        GlobalVariable *fname_global = new GlobalVariable(fname->getType(), true, GlobalValue::InternalLinkage, fname, "", &M);
        fname = ConstantExpr::getGetElementPtr(fname_global, zeros, 2);
        Value *insert_message_function = ConstantExpr :: getIntToPtr(ConstantInt :: get(Type :: Int32Ty, (uint32_t)InsertMessage),
                   PointerType :: get(FunctionType::get(Type :: VoidTy, parameter_types, false), 0));
///        CallInst :: Create(insert_message_function, fname, "", hot_block);
        CallInst::Create(insert_message_function, Args.begin(), Args.end(), "", hot_block);
        BranchInst :: Create(old_entry, hot_block);
      }
      //cout << M;
      return true;
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

