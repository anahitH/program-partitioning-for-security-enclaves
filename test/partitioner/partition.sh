#! /bin/bash

PDG_PATH="/usr/local/lib/libpdg.so"
DG_PATH="/usr/local/lib/libLLVMdg.so"
SVFG_PATH="/usr/local/lib/Svf.so"
SELF_PATH="/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/build/libprogram_partitioning.so"

DIRS=('snake' 'tetris' '2048_game')

run_partition() {
    dir=$1
    cd $dir
    bc="$dir.bc"
    outfile="partition.txt"
    annots="$dir"
    annots+="_annotations.json"
    echo "opt -load $SVFG_PATH -load $DG_PATH -load $PDG_PATH -load $SELF_PATH $bc -partition-analysis -json-annotations=$annots -outfile=$outfile"
    opt -load $SVFG_PATH -load $DG_PATH -load $PDG_PATH -load $SELF_PATH $bc -partition-analysis -json-annotations=$annots -outfile=$outfile

    sort -o $outfile $outfile
    sort -o "expected_partition.txt" "expected_partition.txt"

    if cmp -s "expected_partition.txt" $outfile ; then
        echo "PASS"
    else
        echo "Fail: partition differs from expected one."
    fi
    cd -
}

for dir in "${DIRS[@]}"
do
    run_partition $dir
done
