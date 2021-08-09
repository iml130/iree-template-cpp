# IREE C++ Template

![Build IREE C++ Template](https://github.com/iml130/iree-template-cpp/workflows/Build%20IREE%20C++%20Template/badge.svg)

**DISCLAIMER**:
This project is not intended for everyday use and made available without any support.
However, we welcome any kind of feedback via the issue tracker or if appropriate via IREE's [communication channels](https://github.com/google/iree#communication-channels), e.g. via the Discord server.

The IREE C++ template demonstrates how to integrate [IREE](https://github.com/google/iree) into a third-party project with CMake.
IREE runtime support is demonstrated via the `simple_embedding` sample forked from IREE, where the original source can be found [here](https://github.com/google/iree/tree/master/iree/samples/simple_embedding).
The `custom_modules` sample further demonstrates the out-of-tree usage of a custom dialect alongside with the runtime. The original `custom_modules` source can be found [here](https://github.com/google/iree/tree/main/iree/samples/custom_modules).

The template is developed by Marius Brehler at Fraunhofer IML.

## Getting Started

### Prerequisites

Since this template demonstrates how to use IREE compontens out-of-tree, the prerequisites match those of IREE, documented in the [Getting started](https://google.github.io/iree/building-from-source/getting-started/) guide.
Make sure you have CMake >= 3.13.4 and Clang installed. GCC is not fully supported yet.

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

Explore the samples `simple_embedding` and `custom_modules` within the build directory:

`simple_embedding`
* `simple_embedding_test`: Test executable.

`custom_modules`
* `custom_modules_test`: Test executable.

`custom_modules/dialect`
* `custom-opt`: See the related documentation for [iree-opt](https://github.com/google/iree/blob/main/docs/developers/developing_iree/developer_overview.md#iree-opt) documentation.
* `custom-translate`: See the related documentation for [iree-translate](https://github.com/google/iree/blob/main/docs/developers/developing_iree/developer_overview.md#iree-translate) documentation.

Of course it's also worth to explore the corresponding CMake configuration.
