echo rm -f llvmprof.out .bc .s .o .d
rm -f llvmprof.out $1.bc $1.s $1.o $1.d
echo
echo -------- llvm-g++ --------
llvm-g++ -emit-llvm -c -o $1.bc $1.cpp
echo
echo -------- lli --------
lli -load ../../llvm-2.3/Release/lib/profile_rt.so $1.bc
echo
echo -------- llc --------
llc -f $1.bc
echo
echo -------- multiloops.s --------
cat $1.s
