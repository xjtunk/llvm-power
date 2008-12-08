#!/bin/sh
echo rm -f llvmprof.out .s .o .d
rm -f llvmprof.out $1.s $1.o $1.d $1.cfg.main.ps *.cfg.dot
echo
echo -------- ptlsim and lli --------
sudo ../../llvm-2.3/ptlsim/ptlsim ../../llvm-2.3/Release/bin/lli -load ../../llvm-2.3/Release/lib/profile_rt.so $1.bc
echo
echo -------- llc --------
llc -f $1.bc
echo
echo -------- multiloops.s --------
cat $1.s
echo
echo -------- graphs --------
opt -f -load ../../llvm-2.3/Release/lib/profile_rt.so -profile-loader -print-cfg $1.bc
dot -Tps -o $1.cfg.main.ps cfg.main.dot
evince $1.cfg.main.ps
