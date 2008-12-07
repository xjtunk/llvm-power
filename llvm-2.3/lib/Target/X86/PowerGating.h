#ifndef  __POWERGATING_H__
#define  __POWERGATING_H__

// Ahmad

// Some llvm-power related global declarations, enumerations, etc.

namespace llvm 
{
  enum FunctionUnitType
  {
    FUT_ALL = -1,
    FUT_NONE = 0,
    FUT_INT_ADD_ARITH = 1,
    FUT_INT_ADD_LOGIC,
    FUT_INT_SHIFT,
    FUT_INT_MUL,
    FUT_INT_DIV,
    FUT_FP_ADD,
    FUT_FP_MUL,
    FUT_FP_DIV,
    FUT_FP_SQRT,
    FUT_LOAD,
    FUT_STORE,
    FUT_AGU,
	FUT_BRANCH,
	FUT_MOVE,
	FUT_SET,
	FUT_CMP,
    FUT_L1_DCACHE,
    FUT_L2_UCACHE,
	FUT_MMX,
	FUT_VECTOR_ALU,
	FUT_VECTOR_MUL,
	FUT_VECTOR,
	FUT_END_OF_FUTS
  };
  // 64-bit mask defines which functional units to turn off.
  typedef unsigned long long gatingmask_t; 

  // Added by Brooks
  // This function returns the mask of functional units to turn on
  // It takes a variable argument list of all the FUTs to turn on and returns the gating mask
  gatingmask_t turnOnFUT(int num, ...);
}

#endif  /*__POWERGATING_H__*/
