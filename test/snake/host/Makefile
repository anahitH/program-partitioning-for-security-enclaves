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

CFLAGS=$(shell pkg-config oehost-$(COMPILER) --cflags)
LDFLAGS=$(shell pkg-config oehost-$(COMPILER) --libs)

build:
	@ echo "Compilers used: $(CC), $(CXX)"
	oeedger8r ../sgx.edl.edl --untrusted
	$(CC) -g -c $(CFLAGS) snake.c.c
	$(CC) -g -c $(CFLAGS) sgx.edl_u.c
	$(CC) -o snake.chost sgx.edl_u.o <UNTRUSTED_LIB>.o snake.c.o $(LDFLAGS)

clean:
	rm -f snake.chost snake.c.o sgx.edl_u.o sgx.edl_u.c sgx.edl_u.h sgx.edl_args.h
