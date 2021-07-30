LLVM=/apps/llvm-toolchain/EPI-0.7/development
CLANG=${LLVM}/bin/clang
CLANG_FLAGS="-O2"
GCC=gcc
VEHAVE_PATH=/apps/vehave/EPI-0.7/development/

.PHONY: all clean

all: quicksort quicksort_riscv_opt quicksort_riscv_opt3

quicksort.o: quicksort.c
	${GCC} -c -o quicksort.o quicksort.c

quicksort: quicksort.o
	${GCC}  -o quicksort quicksort.o

clean:
	rm quicksort.o quicksort quicksort_riscv_opt quicksort_riscv_opt.o

quicksort_riscv_opt: quicksort_riscv_opt.o
	${CLANG} ${CLANG_FLAGS}  -o quicksort_riscv_opt quicksort_riscv_opt.o

quicksort_riscv_opt.o: quicksort_riscv_opt.c
	${CLANG} -mepi -c -o quicksort_riscv_opt.o quicksort_riscv_opt.c

quicksort_riscv_opt3: quicksort_riscv_opt3.o
	${CLANG} ${CLANG_FLAGS}  -o quicksort_riscv_opt3 quicksort_riscv_opt3.o

quicksort_riscv_opt3.o: quicksort_riscv_opt3.c
	${CLANG} -mepi -c -o quicksort_riscv_opt3.o quicksort_riscv_opt3.c

run_riscv_opt: quicksort_riscv_opt
	export VEHAVE_DEBUG_LEVEL=0 VEHAVE_TRACE_SINGLE_THREAD=1
	LD_PRELOAD=${VEHAVE_PATH}/lib64/libvehave.so ./quicksort_riscv_opt
