echo rm -f llvmprof.out loop.bc loop.s loop.o loop.d
rm -f llvmprof.out multiloops.bc multiloops.s multiloops.o multiloops.d
echo
echo -------- llvm-g++ --------
llvm-g++ -emit-llvm -c -o multiloops.bc multiloops.cpp
echo
echo -------- lli --------
lli -load ../../Release/lib/profile_rt.so multiloops.bc
echo
echo -------- llc --------
llc -f multiloops.bc
echo
echo -------- multiloops.s --------
cat multiloops.s
