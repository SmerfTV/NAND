#!/bin/bash

# Lista testów do wykonania
tests=("params" "example" "simple" "connect" "adder" "complexity" "input" "output" "line" "circle" "pla" "flipflop" "notnot" "memory")

# Iterowanie po liście testów
for test in "${tests[@]}"; do
    echo "$test"
    time ./nand_tests "$test"
    echo "status = $?"
done
