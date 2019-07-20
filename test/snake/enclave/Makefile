# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Detect C and C++ compiler options
# if not gcc, default to clang-7

COMPILER=$(notdir $(CC))
ifeq ($(COMPILER), gcc)
        USE_GCC = true
endif

ifeq ($(USE_GCC),)
        CC = clang-7
        COMPILER=clang
endif

CFLAGS=$(shell pkg-config oeenclave-$(COMPILER) --cflags)
LDFLAGS=$(shell pkg-config oeenclave-$(COMPILER) --libs)

all:
	$(MAKE) build
	$(MAKE) keys
	$(MAKE) sign

build:
	@ echo "Compilers used: $(CC), $(CXX)"
	oeedger8r ../snake.edl --trusted
	$(CC) -g -c $(CFLAGS) -DOE_API_VERSION=2 snake.c_enclave.c -o snake.c_enclave.o
	$(CC) -g -c $(CFLAGS) -DOE_API_VERSION=2 sgx.edl_t.c -o sgx.edl_t.o
	$(CC) -o snake.cenc sgx.edl_t.o ecnalve_lib.o.o snake.c_enclave.o $(LDFLAGS)

sign:
	oesign sign -e snake.cenc -c enclave.conf -k private.pem

clean:
	rm -f snake.c_enclave.o snake.cenc snake.cenc.signed private.pem public.pem sgx.edl_t.o sgx.edl_t.h sgx.edl_t.c sgx.edl_args.h

keys:
	openssl genrsa -out private.pem -3 3072
	openssl rsa -in private.pem -pubout -out public.pem
