llvm-gcc -emit-llvm -c hello.c -o hello.bc
opt -load=../../Release/lib/LLVMHello.so -hello hello.bc
