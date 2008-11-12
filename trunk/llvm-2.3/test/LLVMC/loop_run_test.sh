echo rm -f llvmprof.out loop.bc loop.s loop.o loop.d
rm -f llvmprof.out loop.bc loop.s loop.o loop.d
echo
echo -------- llvm-g++ --------
llvm-g++ -emit-llvm -c -o loop.bc loop.cpp
echo
echo -------- lli --------
lli -load ../../Debug/lib/profile_rt.so loop.bc
echo
echo -------- llc --------
llc -f loop.bc
echo
echo -------- loop.s --------
cat loop.s
