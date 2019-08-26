# program-partitioning-for-security-enclaves
Behavior based program partitioning for Intel SGX security enclaves. Partitions given program into security sensitive and insensitive partitions based on user annotations. Automatically generates Asylo code to integrate partitioned program with Asylo framework.
Operates in LLVM IR level.

### Dependencies
The following projects need to be built and installed in the system.

1. nlohman json https://github.com/nlohmann/json
2. SVF https://github.com/anahitH/SVF
3. PDG https://github.com/anahitH/program-dependence-graph
4. spdlog https://github.com/gabime/spdlog

For building and installing each of the mentioned projects refer to their github pages.

### Build
mkdir build
cd build
cmake ../
make

Minimum required cmake version is 3.12

### Run

#### Run partitioning

``` opt -load $SVFG_PATH -load $DG_PATH -load $PDG_PATH -load $SELF_PATH $bc -partition-analysis -json-annotations=$annots -outfile=$outfile -partition-stats```

Where ```SVFG_PATH``` is the path where SVF libraries reside. ```PDG_PATH``` is the same for PDG project. ```SELF_PATH``` is the path where program partitioning library files reside. Normally this would be in the ```build``` directory. ```bc``` is the LLVM bitcode of the program to partition and ```annots``` is the json file of user annotations. The statistics of partitioning will be dumped in a file ```partition_stats.json```.

#### Partitioning with optimization
The supported partition optimization methods are:
1. no-opt - no optimization applied
2. ilp - ILP optimization
3. kl - Kernighan-Lin optimization 
4. search-based - optimization based on the static analysis

The default value for optimization is ```no-opt```. In order to optimize the partition set the ```-optimize``` flag of opt. E.g.

``` opt -load $SVFG_PATH -load $DG_PATH -load $PDG_PATH -load $SELF_PATH $bc -partition-analysis -json-annotations=$annots -outfile=$outfile -optimize=[|ilp|kl|search-based] -partition-stats```

### Generating secure and insecure modules from partition
``` opt -load $SVFG_PATH -load $DG_PATH -load $PDG_PATH -load $SELF_PATH $bc -extract-partition -json-annotations=$annots -outfile=$outfile -optimize=[|ilp|kl|search-based] -partition-stats```

Will generate two modules out of two partitions. Will add missing code to have funcional modules, e.g. setter functions for globals used and modified in both partitions.

#### Asylo code generation

``` $BIN_PATH/sgx-code-gen $SRC ```
where ```BIN_PATH``` is where executeable ```sgx-code-gen``` resides. By default it should be in the directory ```build```.
```SRC``` is list of source files the program constsis of. Those are source files in C and not LLVM IR.

The generated files will be:
1. interface_selectors.h
2. ${program_name}\_enclave.cc
3. ${program_name}\_driver.cc

#### To generate Bazel build file for given pprogram

```python scripts/generateBazelBuild.py -enclave $enclave_shared_lib_name -enclave_src $enclave.cc -enclave_lib $secure_partition.o -enclave_hdr $enclave_hdr_files -app $app_name $app_src $driver.cc -app_lib $insecure_partition.o -app_hdr $app_hdrs_files -enclaves $enclaves_name -build-template $BAZEL_build_template```

Bazel build template file is in ```templates/BUILD_template```.

#### Building docker image
To start the docker image build make sure to download IBM CPLEX studio installer and copy it to the ```docker_setup``` directory.
To build a docker image with Asylo installed in it run:
``` cd docker_setup
    ./setup_docker.sh ```
