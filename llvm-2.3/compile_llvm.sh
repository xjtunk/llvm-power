# This shell script compiles a macro-benchmark.
llvm-gcc -c -emit-llvm *.c
llvm-g++ -c -emit-llvm *.cpp
llvm-ld *.o
