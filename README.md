# IREE C++ Template

![Build IREE C++ Template](https://github.com/iml130/iree-template-cpp/workflows/Build%20IREE%20C++%20Template/badge.svg)

**DISCLAIMER**:
This project is not intended for everyday use and made available without any support.
However, we welcome any kind of feedback via the issue tracker or if appropriate via IREE's [communication channels](https://github.com/iree-org/iree#communication-channels), e.g. via the Discord server.

The IREE C++ template demonstrates how to integrate [IREE](https://github.com/iree-org/iree) into a third-party project with CMake.
IREE runtime support is demonstrated via the `iree_simple_embedding` sample forked from IREE, where the original source can be found [here](https://github.com/iree-org/iree/tree/master/samples/iree_simple_embedding).

The template is developed by Marius Brehler at Fraunhofer IML.

## Getting Started

### Prerequisites

Since this template demonstrates how to use IREE compontens out-of-tree, the prerequisites match those of IREE, documented in the [Getting started](https://iree-org.github.io/iree/building-from-source/getting-started/) guide.
Make sure you have CMake >= 3.16.3 and clang installed.

### Clone and Build
#### Clone

```shell
git clone https://github.com/iml130/iree-template-cpp.git
cd iree-template-cpp
git submodule update --init
cd third_party/iree
git submodule update --init
cd ../../
```
> Note:<br>
> &nbsp;&nbsp;&nbsp;&nbsp;The submodules used within IREE themself include submodules, so that we advice to avoid an recursive update.

> Note:<br>
> &nbsp;&nbsp;&nbsp;&nbsp;It may happen that new submodules are added to IREE.<br>
> &nbsp;&nbsp;&nbsp;&nbsp;Thus it might not be sufficient to only pull the latest master and you may need update the submodules manually.

#### Configure and Build

```shell
# export LDFLAGS=-fuse-ld=$(which ld.lld)

mkdir build
cd build
cmake -GNinja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .. \
# -DLLVM_ENABLE_LLD=ON

ninja
```
> Note:<br>
> &nbsp;&nbsp;&nbsp;&nbsp;Uncomment the lines above to enable linking via ldd.

#### What's next?

Explore the sample `iree_simple_embedding` within the build directory:

`iree_simple_embedding`
* `simple_embedding_test`: Test executable.

Of course it's also worth to explore the corresponding CMake configuration.
