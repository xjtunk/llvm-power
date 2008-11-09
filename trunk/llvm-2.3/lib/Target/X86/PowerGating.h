#ifndef  __POWERGATING_H__
#define  __POWERGATING_H__

#include <cstdarg>

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
    FUT_INT_AGU, // Address generation unit. Might be merged with adder
	FUT_BRANCH,
	FUT_MOVER,
	FUT_SET,
	FUT_LOGIC,
	FUT_CMP,
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

  // Added by Brooks
  // This function returns the mask of functional units to turn on
  // It takes a variable argument list of all the FUTs to turn on and returns the gating mask
  gatingmask_t turnOnFUT(int num, ...) {
    // If there are no FUTs to turn on...
    if (num == FUT_NONE) {
      return (gatingmask_t)0;
	}
	// If we want to turn on all the FUTs...
	else if (num == FUT_ALL) {
      return (gatingmask_t)0 - 1;
	}
	// If we want to turn on some FUTs...
	else {
      va_list ap; //argument pointer
	  va_start(ap, num);
	  gatingmask_t newmask = 0;
	  for (int i = 0; i < num; i++) {
        int unit = va_arg(ap, int);
        newmask |= ((gatingmask_t)1 << unit);
	  }
      va_end(ap);
      return newmask;
	}
  }
}

#endif  /*__POWERGATING_H__*/
