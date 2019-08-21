#!/bin/bash

PDG_PATH="/usr/local/lib/libpdg.so"
SVFG_PATH="/usr/local/lib/Svf.so"
SELF_PATH="$PWD/../../build/libprogram_partitioning.so"

BITCODES=$PWD/dataset/*.bc
ANNOTATIONS=$PWD/annotations/
OUTPUT=$PWD/partition-out

partition() {
    bitcode=$1
    annots=$2
    optimization=$3
    echo "Run for $bitcode with optimization $optimization"
    echo "opt -load $SVFG_PATH -load $PDG_PATH -load $SELF_PATH $bc -partition-analysis -json-annotations=$annots -outfile=$outfile"
    opt -load $SVFG_PATH -load $PDG_PATH -load $SELF_PATH $bc -partition-analysis -optimize="$optimization" -json-annotations=$annots -partition-stats 
}

run-optimization() {
    opt=$1
    for bc in $BITCODES
    do
	bitcode=$bc
        echo $bitcode
	filename=${bc##*/}
	filename=${filename::-3}
        annotation=$ANNOTATIONS/$filename"_annotations.json"
        output_dir=$OUTPUT/$filename
        mkdir -p $output_dir
        output_dir=$output_dir/$opt
        mkdir -p $output_dir
        cd $output_dir
        partition $bitcode $annotation $opt
        cd ../../
    done
}

optimization=$1
optimizations=('' 'kl' 'local' 'static-analysis' 'ilp')
if [ "$optimization" == "all" ]; then
    for opt in "${optimizations[@]}"
    do
        echo $opt
        run-optimization $opt
    done
else
    run-optimization $optimization
fi

echo 'Partitioning is done!'

