llvm-gcc -emit-llvm -c hello.c -o hello.bc
# If you are using debug build, use this one
opt -load=../../Debug/lib/LLVMHello.dylib -hello hello.bc
# else use the standard one
opt -load=../../Release/lib/LLVMHello.so -hello hello.bc
