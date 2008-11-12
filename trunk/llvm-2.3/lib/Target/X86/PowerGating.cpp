#include "PowerGating.h"
#include <cstdarg>

namespace llvm {
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

