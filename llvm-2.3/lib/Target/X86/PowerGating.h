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
    FUT_INT_ADDER_ARITH = 1,
    FUT_INT_ADDER_LOGIC,
    FUT_INT_SHIFTER,
    FUT_INT_MULTIPLIER,
    FUT_INT_DIV,
    FUT_INT_AGU, // Address generation unit. Might be merged with adder
	FUT_BRANCH,
	FUT_MOVER,
	FUT_SET,
	FUT_LOGIC,
	FUT_CMP,
    FUT_FP_ADDER,
    FUT_FP_MULTIPLIER,
    FUT_FP_DIV,
    FUT_FP_MADD,
    FUT_FP_SQRT,
	FUT_FP_ANDOR,
    FUT_LOAD,
    FUT_STORE,
    FUT_L1_DCACHE,
    FUT_L2_UCACHE
  };
  // 64-bit mask defines which functional units to turn off.
  typedef unsigned long long gatingmask_t; 

  // Added by Brooks
  // This function returns the mask of functional units to turn on
  // It takes a variable argument list of all the FUTs to turn on and returns the gating mask
  gatingmask_t turnOnFUT(int num, ...);
}

#endif  /*__POWERGATING_H__*/
