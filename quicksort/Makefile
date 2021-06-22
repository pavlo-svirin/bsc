LLVM=/apps/llvm-toolchain/EPI-0.7/development
CLANG=${LLVM}/bin/clang
CLANG_FLAGS="-O2 -mepi -fno-vectorize"
GCC=gcc

all: quicksort

quicksort.o: quicksort.c
	${GCC} -c -o quicksort.o quicksort.c

quicksort: quicksort.o
	${GCC}  -o quicksort quicksort.o

clean:
	rm quicksort.o quicksort


run:
	;
