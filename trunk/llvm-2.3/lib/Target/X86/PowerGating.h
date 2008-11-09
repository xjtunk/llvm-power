#ifndef  __POWERGATING_H__
#define  __POWERGATING_H__

// Ahmad

// Some llvm-power related global declarations, enumerations, etc.

namespace llvm 
{
  enum FunctionUnitType
  {
    FUT_INT_ADDER = 1,
    FUT_INT_SHIFTER,
    FUT_INT_MULTIPLIER,
    FUT_INT_AGU, // Address generation unit. Might be merged with adder
	FUT_INT_ANDOR,
	FUT_BRANCH,
    FUT_FP_ADDER,
    FUT_FP_MULTIPLIER,
    FUT_FP_MADD,
    FUT_FP_SQRT,
	FUT_FP_ANDOR,
    FUT_LD_QUEUE,
    FUT_ST_QUEUE,
    FUT_L1_DCACHE,
    FUT_L2_UCACHE
  };
  // 64-bit mask defines which functional units to turn off.
  typedef unsigned long long gatingmask_t; 
}

#endif  /*__POWERGATING_H__*/
