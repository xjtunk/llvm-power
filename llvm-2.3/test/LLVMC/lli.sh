###lli -load ../../Debug/lib/profile_rt.so -print-machineinstrs -x86-asm-syntax=intel hello.bc
###lli -load ../../Debug/lib/profile_rt.so -x86-asm-syntax=intel hello.bc
lli -load ../../Debug/lib/profile_rt.so -x86-asm-syntax=intel tracetest.bc

