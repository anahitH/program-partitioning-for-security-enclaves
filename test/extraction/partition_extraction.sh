#! /bin/bash

PDG_PATH="/usr/local/lib/libpdg.so"
DG_PATH="/usr/local/lib/lib/libLLVMdg.so"
SVFG_PATH="/usr/local/lib/Svf.so"
SELF_PATH="/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/build/libprogram_partitioning.so"

DIRS=('snake')
#DIRS=('snake' 'tetris' '2048_game' 'memcached')

#opt -load /usr/local/lib/Svf.so -load /usr/local/lib/lib/libLLVMdg.so -load /usr/local/lib/libpdg.so -load /home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/build/libprogram_partitioning.so snake.bc -partition-analysis -json-annotations=snake_annotations.json -outfile=partition.txt
run_partition() {
    dir=$1
    cd $dir
    bc="$dir.bc"
    outfile=$dir
    outfile+="_partition.json"
    annots="$dir"
    annots+="_annotations.json"
    echo "opt -load $SVFG_PATH -load $DG_PATH -load $PDG_PATH -load $SELF_PATH $bc -extract-partition -optimize="static-analysis" -json-annotations=$annots -partition-stats -outfile=$outfile"
    opt -load $SVFG_PATH -load $DG_PATH -load $PDG_PATH -load $SELF_PATH $bc -extract-partition -optimize="static-analysis" -json-annotations=$annots -partition-stats -stats-file=$outfile
    cd -
}

for dir in "${DIRS[@]}"
do
    run_partition $dir
done
