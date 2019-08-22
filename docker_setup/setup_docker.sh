#!/bin/bash

# clone asylo
git clone https://github.com/google/asylo.git

# copy asylo toolchain to $PWD
cp -r asylo/asylo/distrib/toolchain/ ./
cp Dockerfile ./toolchain
cp COSCE129LIN64.bin ./toolchain
cp installer.properties ./toolchain

cd ./toolchain

# build docker images
sudo docker build -t program-partitioning-for-security-enclaves .

