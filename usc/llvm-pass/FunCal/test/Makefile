# Makefile for tests; generates unoptimized .ll IR for all tests

# Test files
FILES = test.bc

##
LLVM_ROOT = ../../..
CLANG = $(LLVM_ROOT)/Debug+Asserts/bin/clang
CLANG_FLAGS = -emit-llvm -O0 -g

all: $(FILES)

clean:
	rm -f *.bc *.ll *.s $(FILES:%.bc=%)

test.bc: test.c
	$(CLANG) $(CLANG_FLAGS) -c test.c -o test.bc
