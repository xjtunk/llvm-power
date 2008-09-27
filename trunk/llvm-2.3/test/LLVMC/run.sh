llvm-gcc -emit-llvm -c hello.c
opt -load=../../Release/lib/LLVMHello.so -hello hello.bc
