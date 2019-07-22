#! /bin/bash

# LIBRARY PATHS
PDG_PATH="/usr/local/lib/libpdg.so"
SVFG_PATH="/usr/local/lib/Svf.so"
SELF_PATH="/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/build/libprogram_partitioning.so"
CODE_GEN_BIN="/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/build/sgx-code-gen"
BUILDFILE_GEN_SCRIPTS="/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/scripts"
TEMPLATE_FILES="/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/templates/"
OPENENCLAVE_UTILS="/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/open_enclave_utils/app_utils.h"

#directory where the program to protected is
dir=$1
# program to protect
program=$2
# bitcode of the program
bc=$3
# user annotations
annots=$4
# framework to use
framework=$5
optimization=$6

enclave_bc="enclave_lib.bc"
app_bc="app_lib.bc"
enclave_lib="enclave_lib.o"
app_lib="app_lib.o"

compile_source() {
    cd dir
    clang -c -emit-llvm $program -o "$program.bc"
    bc="$program.bc"
    cd -
}

process_arguments() {
    echo "Processing arguments"
    correct_args=1
    if [ "$dir" == '' ]; then
        echo "Directory of the program to be protected is not given. Stop execution."
        correct_args=0
    elif [ "$program" == '' ]; then
        echo "Program to be protected is not given. Stop execution."
        correct_args=0;
    elif [ "$bc" == '' ]; then
        echo "Bitcode of the program to be protected is not given. Compiling the program."
        compile_source
    elif [ "$annots" == '' ]; then
        echo "User annotations are not given. Stop execution"
        correct_args=0;
    elif [ "$framework" == '' ] || [ "$framework" != 'asylo' ] && [ "$framework" != 'open-enclave' ]; then
        echo "Wrong framework name. Choosing Asylo as default"
        framework='asylo'
    elif [ "$optimization" == '' ]; then
        echo "No optimization provided. Choosing ILP as default"
        optimization='ilp'
    fi
}

partition() {
    echo "Run partitioning"
    cd $dir
    stats_file="partition-stats.json"
    echo "opt -load $SVFG_PATH -load $PDG_PATH -load $SELF_PATH $bc -extract-partition -optimize="$optimization" -json-annotations="$annots" -partition-stats"
    #RUNNING THE PARTITIONING AND PARTITION EXTRACTION
    opt -load $SVFG_PATH -load $PDG_PATH -load $SELF_PATH $bc -extract-partition -optimize="$optimization" -json-annotations=$annots -partition-stats -stats-file=$stats_file
}

generate_framework_code() {
    echo "Generate framework code for $framework"
    # GENERATING ASYLO CODE
    echo "$CODE_GEN_BIN $program -partition-stats=$stats_file -prefix=$dir -framework=$framework --"
    $CODE_GEN_BIN $program -partition-stats=$stats_file -prefix=$dir -framework=$framework --
}

compile_partitions_to_object() {
    echo "Compile partitioned libraries to object files"
    llc $enclave_bc -o enclave_lib.s
    g++ -std=c++0x -fPIC -c enclave_lib.s -o $enclave_lib
    llc $app_bc -o app_lib.s
    g++ -std=c++0x  -fPIC -c app_lib.s -o $app_lib
}

generate_asylo_code() {
    # GENERATING BUILD FILE
    enclave=$dir
    enclave+="_enclave"
    app=$dir
    app+="_driver"
    echo "python $BUILDFILE_GEN_SCRIPTS/generateBazelBuildFile.py -enclave=$enclave -enclave_src=$enclave.cc -enclave_lib="$enclave_lib" -app=$dir -app_src=$app.cc -app_lib="$app_lib" -enclaves=$dir -build_template=$TEMPLATE_FILES"
    python $BUILDFILE_GEN_SCRIPTS/generateBazelBuildFile.py -enclave=$enclave -enclave_src=$enclave.cc -enclave_lib="$enclave_lib" -app=$dir -app_src=$app.cc -app_lib="$app_lib" -enclaves=$dir -build_template=$TEMPLATE_FILES
}

generate_open_enclave_code() {
    program_name=${program%.*}
    echo $program_name
    echo "python $BUILDFILE_GEN_SCRIPTS/generateOpenEnclaveMakefiles.py -makefile=$TEMPLATE_FILES/Makefile -enclave-makefile=$TEMPLATE_FILES/enclave_Makefile -host-makefile=$TEMPLATE_FILES/host_Makefile -program=$program -enclave_lib=$enclave_lib -unprotected_lib=$app_lib -edl=sgx.edl"
    python $BUILDFILE_GEN_SCRIPTS/generateOpenEnclaveMakefiles.py -makefile=$TEMPLATE_FILES/Makefile -enclave-makefile=$TEMPLATE_FILES/enclave_Makefile -host-makefile=$TEMPLATE_FILES/host_Makefile -program=$program_name -enclave_lib=${enclave_lib%.*} -unprotected_lib=${app_lib%.*} -edl=sgx
    mkdir enclave
    mkdir host
    mv $TEMPLATE_FILES/enclave_Makefile_patched enclave/Makefile
    mv $dir"_enclave.cc" enclave
    mv $enclave_lib enclave
    cp $TEMPLATE_FILES/enclave.conf enclave
    mv $TEMPLATE_FILES/host_Makefile_patched host/Makefile
    mv $dir"_app.cc" host
    mv $app_lib host
    mv $TEMPLATE_FILES/Makefile_patched Makefile
    cp $OPENENCLAVE_UTILS host
}

generate_build_code() {
    echo "Generate build files"
    if [ "$framework" == "asylo" ]; then
        generate_asylo_code
    else
        generate_open_enclave_code
    fi
}

build_enclave_app() {
    echo "build enclave app for $framework"
    if [ "$framework" == "asylo" ]; then
        echo "build for Asylo"
        #build enclave
        bazel build :"$enclave.so" --config=asylo --define=ASYLO_SIM=1
        #build driver
        bazel build :$app --define=ASYLO_SIM=1
    else
        echo "build for OpenEnclave"
        make build
    fi
}

process_arguments
if [ $correct_args == 1 ]; then
    echo "Protecting program $dir/$program with bitcode $bc, annotations $annots, optimizing partitioning with $optimization for framework $framework"
    partition
    compile_partitions_to_object
    generate_framework_code
    generate_build_code
    build_enclave_app
fi

