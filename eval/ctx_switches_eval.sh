#!/bin/bash

PDG_PATH="/usr/local/lib/libpdg.so"
DG_PATH="/usr/local/lib/lib/libLLVMdg.so"
SVFG_PATH="/usr/local/lib/Svf.so"
PROGRAM_PARTITIONING_PATH="/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/build/libprogram_partitioning.so"
PROGRAM_PARTITIONING_EVAL_PATH="/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/build/libeval-program_partitioning.so"

BITCODES=$PWD/dataset/*.bc
ANNOTATIONS=$PWD/annotations/
OUTPUT=$PWD/partition-out

annotation_coverage="10 25 35 50"
optimization=$1
#optimizations=('no-opt' 'kl' 'ilp' 'local')
optimizations=('no-opt' 'kl' 'local' 'static-analysis' 'ilp')
run_partitioning=$1

set_manual_annotation() {
    echo 'Setup manual annotations directory for ' $1
    bitcode=$1
    filename=$2
    manual_annot_dir=$3/'manual'
    mkdir -p $manual_annot_dir
    cp $ANNOTATIONS/$filename'_annotations.json' $manual_annot_dir
    echo 'DONE Setup manual annotations directory for ' $1
}

generate_annotations_for_program() {
    echo 'Generating random annotations for ' $1
    bitcode=$1
    filename=$2
    annot_output_dir=$3/'annotations'
    mkdir -p $annot_output_dir
    cd $annot_output_dir
    for cov in $annotation_coverage
    do
        echo '  percentage: ' $cov
        annot_name=$filename-$cov'.json'
        echo 'opt -load $PROGRAM_PARTITIONING_EVAL_PATH $bc -gen-annotation -percent=$cov -annotation-file=$annot_name'
        opt -load $PROGRAM_PARTITIONING_EVAL_PATH $bc -gen-annotation -percent=$cov -annotation-file=$annot_name
        cov_dir=$3/$cov
        mkdir -p $cov_dir
        cp $annot_output_dir/$annot_name $cov_dir
    done
    cd -
    rm -rf $annot_output_dir
    echo 'DONE: Generating random annotations for ' $1
}

run_partitioning_for_optimization() {
    echo 'Run partitioning for optimization ' $1
    opt=$1
    bitcode=$2
    annot=$3
    opt_output_dir=$4/$opt
    #echo "ARGS: " $1 $2 $3 $4
    mkdir -p $opt_output_dir
    cp $annot $opt_output_dir
    cd $opt_output_dir
    echo 'opt -load $SVFG_PATH -load $DG_PATH -load $PDG_PATH -load $PROGRAM_PARTITIONING_PATH $bitcode -partition-analysis -optimize="$opt" -json-annotations=$annot -partition-stats'
    if [ "$opt" == "no-opt" ]; then
        opt -load $SVFG_PATH -load $DG_PATH -load $PDG_PATH -load $PROGRAM_PARTITIONING_PATH $bitcode -partition-analysis -json-annotations=$annot -partition-stats
    else
        opt -load $SVFG_PATH -load $DG_PATH -load $PDG_PATH -load $PROGRAM_PARTITIONING_PATH $bitcode -partition-analysis -optimize="$opt" -json-annotations=$annot -partition-stats
    fi
    cd -
    echo 'DONE: Run partitioning for optimization ' $1
}

run_partitioning_for_coverage() {
    echo 'Run partitioning for coverage ' $1
    bitcode=$2
    annot_name=$3
    #echo "ARGS: " $1 $2 $3 $4
    if [ "$optimization" == "all" ]; then
        for opt in "${optimizations[@]}"
        do
            #echo "HERE OPT is " $opt 'and annot ' $annot_name
            run_partitioning_for_optimization $opt $bitcode $annot_name $4/$1
        done
    else
        run_partitioning_for_optimization $optimization $bitcode $annot_name $4/$1
    fi
    echo 'DONE: Run partitioning for coverage ' $1
}

run_partitioning() {
    echo 'Run partitioning for ' $1
    bitcode=$1
    filename=$2
    dir=$3/
    current_dir=$PWD
    #echo "ARGS: " $1 $2 $3
    for cov in $annotation_coverage
    do
        annot_dir=$dir/$cov
        mkdir -p $annot_dir
        cd $annot_dir
        annot_name=$filename-$cov'.json'
        #echo "run_partitionig annot_name is " $annot_name
        run_partitioning_for_coverage $cov $bitcode $annot_name $dir
        cd $current_dir
    done
    cd $dir/'manual'
    run_partitioning_for_coverage 'manual' $bitcode $filename'_annotations.json' $dir
    cd $current_dir
    echo 'DONE: Run partitioning for ' $1
}

run_partitioning() {
    # for each program
    for bc in $BITCODES
    do
	bitcode=$bc
        echo 'Running partitioning for' $bitcode
	filename=${bc##*/}
	filename=${filename::-3}
        output_dir=$OUTPUT/$filename
        mkdir -p $output_dir
        generate_annotations_for_program $bitcode $filename $output_dir
        set_manual_annotation $bitcode $filename $output_dir
        run_partitioning $bitcode $filename $output_dir
    done

}

generate_instrumented_binaries_for_shadow_call_stack_for_bitcode_and_coverage_and_opt() {
    echo 'generate_instrumented_binaries_for_shadow_call_stack_for_bitcode_and_coverage_and_opt ' $1
    opt=$1
    bitcode=$2
    filename=$4
    echo $1 $2 $3 $4
    cd $3/$opt
    echo 'opt -load $PROGRAM_PARTITIONING_EVAL_PATH $bc -gen-shadow-stack -o $3/$opt/$filename"_instrumented.bc'
    opt -load $PROGRAM_PARTITIONING_EVAL_PATH $bc -gen-shadow-stack -o $3/$opt/$filename'_instrumented.bc'
    cd -
    echo 'DONE: generate_instrumented_binaries_for_shadow_call_stack_for_bitcode_and_coverage_and_opt ' $1
}

generate_instrumented_binaries_for_shadow_call_stack_for_bitcode_and_coverage() {
    echo 'generate_instrumented_binaries_for_shadow_call_stack_for_bitcode_and_coverage ' $1
    echo $1 $2 $3 $4
    bitcode=$2
    if [ "$optimization" == "all" ]; then
        for opt in "${optimizations[@]}"
        do
            #echo "HERE OPT is " $opt 'and annot ' $annot_name
            generate_instrumented_binaries_for_shadow_call_stack_for_bitcode_and_coverage_and_opt $opt $bitcode $3 $4
        done
    else
        generate_instrumented_binaries_for_shadow_call_stack_for_bitcode_and_coverage_and_opt $optimization $bitcode $3 $4
    fi
    echo 'DONE: generate_instrumented_binaries_for_shadow_call_stack_for_bitcode_and_coverage ' $1

}

generate_instrumented_binaries_for_shadow_call_stack_for_bitcode() {
    echo 'Run generate_instrumented_binaries_for_shadow_call_stack for bitcode ' $1
    bitcode=$1
    filename=$2
    dir=$3/
    current_dir=$PWD
    for cov in $annotation_coverage
    do
        annot_dir=$dir/$cov
        cd $annot_dir
        generate_instrumented_binaries_for_shadow_call_stack_for_bitcode_and_coverage $cov $bitcode $annot_dir $filename
        cd $current_dir
    done
    cd $dir/'manual'
    generate_instrumented_binaries_for_shadow_call_stack_for_bitcode_and_coverage 'manual' $bitcode $dir/'manual' $filename
    cd $current_dir

    echo 'DONE: Run generate_instrumented_binaries_for_shadow_call_stack for bitcode ' $1
}

generate_instrumented_binaries_for_shadow_call_stack() {
    echo 'Run generate_instrumented_binaries_for_shadow_call_stack'
    for bc in $BITCODES
    do
	bitcode=$bc
	filename=${bc##*/}
	filename=${filename::-3}
        output_dir=$OUTPUT/$filename
        generate_instrumented_binaries_for_shadow_call_stack_for_bitcode $bc $filename $output_dir
    done
    echo 'DONE: Run generate_instrumented_binaries_for_shadow_call_stack'
}

if [ "$run_partitioning" == "true" ]; then
    echo 'Runing partitioning'
    run_partitioning
    echo 'Partitioning is done!'
fi
generate_instrumented_binaries_for_shadow_call_stack


