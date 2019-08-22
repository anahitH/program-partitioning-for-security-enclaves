#
# Copyright 2018 Asylo authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Use fixed snapshot of Debian to create a deterministic environment.
# Snapshot tags can be found at https://hub.docker.com/r/debian/snapshot/tags
ARG debian_snapshot=buster-20190708

# Start with a temporary image just for building the toolchain.
FROM debian/snapshot:${debian_snapshot} as toolchain

# Add build dependencies from Debian.
RUN apt-get update && \
    apt-get install -y \
        bison \
        build-essential \
        flex \
        libisl-dev \
        libmpc-dev \
        libmpfr-dev \
        rsync \
        texinfo \
        wget \
        zlib1g-dev

COPY . /opt/asylo/distrib/toolchain/

# Build and install the toolchain.
RUN /opt/asylo/distrib/toolchain/install-toolchain \
    --system \
    --prefix /opt/asylo/toolchains/default

## Now, create the final image.
FROM debian/snapshot:${debian_snapshot}

# Use a fixed version of Bazel.
ARG bazel_version=0.28.1
ARG bazel_sha=238795b1850d4c155c8f280828c344c22d096756d206b8be2ba423731dca52c0
ARG bazel_url=https://storage.googleapis.com/bazel-apt/pool/jdk1.8/b/bazel/bazel_${bazel_version}_amd64.deb

# Install development tools
RUN apt-get update && \
    apt-get install -y wget && \
    wget "${bazel_url}" -nv -o- -O bazel.deb && \
    echo "${bazel_sha}  bazel.deb" > bazel.sha256 && \
    sha256sum --check bazel.sha256 && \
    apt-get install -y \
        ./bazel.deb \
        build-essential \
        git \
        libfl2 \
        ocaml-nox \
        ocamlbuild \
        python3 \
        vim \
        && \
    rm bazel.deb bazel.sha256 && \
    apt-get clean && \
    echo ". /etc/bash_completion" >> /root/.bashrc

# Copy the built toolchain from the earlier image.
COPY --from=toolchain /opt/asylo/distrib/toolchain/ /opt/asylo/distrib/toolchain/
COPY --from=toolchain /opt/asylo/toolchains/ /opt/asylo/toolchains/
COPY --from=toolchain /usr/local/share/asylo/ /usr/local/share/asylo/

# Default command to run if not specified otherwise.
CMD ["/bin/bash"]

RUN ls /opt/asylo/distrib/toolchain/COSCE129LIN64.bin

# Install llvm/clang development libraries
RUN apt-get install -y llvm-dev libllvm-7-ocaml-dev libllvm7 llvm-7 llvm-7-dev llvm-7-doc llvm-7-runtime \
        clang-7 clang-tools-7 clang-7-doc libclang-common-7-dev libclang-7-dev libclang1-7 clang-format-7 \
        lldb-7 lld-7 libc++-7-dev libc++abi-7-dev libomp-7-dev

RUN apt-get install -y libboost-all-dev

# copy clang and llvm libraries and includes
RUN rm /usr/lib/libLTO.so
RUN cp /usr/lib/llvm-7/lib/lib* /usr/lib
RUN cp -r /usr/lib/llvm-7/include/clang/* /usr/include/clang/

WORKDIR /home/

# install cmake-3.12
RUN wget http://www.cmake.org/files/v3.12/cmake-3.12.1.tar.gz && \
    tar -xvzf cmake-3.12.1.tar.gz && \
    cd cmake-3.12.1 && \
    ./configure && \
    make install

RUN cd /home/
#setup CPLEX
# user interaction is required
RUN ls /opt/asylo/distrib/toolchain/COSCE129LIN64.bin
RUN /opt/asylo/distrib/toolchain/COSCE129LIN64.bin -f "./installer.properties"
RUN cp /opt/ibm/ILOG/CPLEX_Studio_Community129/concert/lib/x86-64_linux/static_pic/libconcert.a /usr/lib && \
         cp /opt/ibm/ILOG/CPLEX_Studio_Community129/cplex/lib/x86-64_linux/static_pic/libcplex.a /usr/lib/ && \
         cp /opt/ibm/ILOG/CPLEX_Studio_Community129/cplex/lib/x86-64_linux/static_pic/libilocplex.a /usr/lib/ && \
         cp /opt/ibm/ILOG/CPLEX_Studio_Community129/opl/lib/x86-64_linux/static_pic/libcp.a /usr/lib/


#clone build install nlohman json
RUN git clone https://github.com/nlohmann/json.git && \
    mkdir -p /home/json/build && \
    cmake -H/home/json/ -B/home/json/build &&\
    make -C /home/json/build install

#clone build install spdlog
RUN git clone https://github.com/gabime/spdlog.git && \
    mkdir -p /home/spdlog/build && \
    cmake -H/home/spdlog/ -B/home/spdlog/build &&\
    make -C /home/spdlog/build install

RUN cd /home/

#clone build install SVF
RUN git clone https://github.com/anahitH/SVF.git && \
    mkdir -p /home/SVF/build && \
    cmake -H/home/SVF/ -B/home/SVF/build &&\
    make -C /home/SVF/build install

#clone build and install PDG    
RUN git clone --depth 1 -b docker https://github.com/anahitH/program-dependence-graph.git && \
    mkdir -p /home/program-dependence-graph/build && \
    cmake -H/home/program-dependence-graph/ -B/home/program-dependence-graph/build &&\
    make -C /home/program-dependence-graph/build install

#program-partitioning
RUN git clone --depth 1 -b asylo-docker https://github.com/anahitH/program-partitioning-for-security-enclaves.git && \
    mkdir -p /home/program-partitioning-for-security-enclaves/build && \
    cmake -H/home/program-partitioning-for-security-enclaves/ -B/home/program-partitioning-for-security-enclaves/build &&\
    make -C /home/program-partitioning-for-security-enclaves/build
