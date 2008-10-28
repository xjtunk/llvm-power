#ifndef  __POWERGATING_H__
#define  __POWERGATING_H__

// Some llvm-power related global declarations, enumerations, etc.

namespace llvm 
{
  // 64-bit mask defines which functional units to turn off.
  typedef unsigned long long gatingmask_t; 
}



#endif  /*__POWERGATING_H__*/
