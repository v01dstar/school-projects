#!/bin/bash

LLVM_ROOT=../../..
OPT=$LLVM_ROOT/Debug+Asserts/bin/opt
PASS_PATH=$LLVM_ROOT/Debug+Asserts/lib
OPT_FLAGS=-load
TEST_CASE=test.bc

$OPT $OPT_FLAGS $PASS_PATH/FunCal.so -funcal <$TEST_CASE> /dev/null
