# Licensed under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# IREE requires a minimum CMake version of 3.13.4
cmake_minimum_required(VERSION 3.13.4)

#-------------------------------------------------------------------------------
# Project configuration
#-------------------------------------------------------------------------------

project(iree-template-cpp VERSION 1.0 LANGUAGES CXX C)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# NOTE: The RTTI setting must match what LLVM was compiled with and IREE
#       defaults to RTTI disabled in `iree_copts.cmake`.
#       RTTI is disabled for `dialect` target, but you may want to consider to
#       to pass `-fno-rtti` as a global compile option by adding the line
# add_compile_options(-fno-rtti)

#-------------------------------------------------------------------------------
# Third-party dependencies
#-------------------------------------------------------------------------------

# Extend module path to allow submodules to find AddMLIR
list(APPEND CMAKE_MODULE_PATH "${PROJECT_BINARY_DIR}/lib/cmake/mlir")

add_subdirectory(third_party/iree EXCLUDE_FROM_ALL)

#-------------------------------------------------------------------------------
# Top-level components
#-------------------------------------------------------------------------------

add_subdirectory(iree_simple_embedding)
