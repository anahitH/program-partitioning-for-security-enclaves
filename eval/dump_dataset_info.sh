#!/bin/bash

PROGRAM_PARTITIONING_EVAL_PATH="/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/build/libeval-program_partitioning.so"
BITCODES=$PWD/dataset/*.bc

for bc in $BITCODES
do
    filename=${bc##*/}
    echo "Program info for: " $filename
    echo 'opt -load $PROGRAM_PARTITIONING_EVAL_PATH $bc -module-info'
    opt -load $PROGRAM_PARTITIONING_EVAL_PATH $bc -module-info
done

