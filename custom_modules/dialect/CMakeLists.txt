# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#-------------------------------------------------------------------------------
# Create a library that contains the custom dialect.
#-------------------------------------------------------------------------------

add_library(dialect STATIC "")
target_sources(dialect
  PRIVATE
    "conversion_patterns.h"
    "custom_dialect.h"
    "custom_ops.h.inc"
    "conversion_patterns.cc"
    "custom_dialect.cc"
    "custom_ops.cc.inc"
)

target_include_directories(dialect
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${iree_SOURCE_DIR}
    ${iree_SOURCE_DIR}/third_party/llvm-project/llvm/include
    ${iree_SOURCE_DIR}/third_party/llvm-project/mlir/include
    ${iree_BINARY_DIR}
    ${iree_BINARY_DIR}/third_party/llvm-project/llvm/include
    ${iree_BINARY_DIR}/third_party/llvm-project/llvm/tools/mlir/include
)

target_link_libraries(dialect
  custom_imports
  LLVMSupport
  MLIRIR
  MLIRParser
  MLIRPass
  MLIRSideEffects
  MLIRTransforms
  iree_compiler_Dialect_HAL_Conversion_Conversion
  iree_compiler_Dialect_HAL_IR_IR
  iree_compiler_Dialect_IREE_IR_IR
  iree_compiler_Dialect_VM_Conversion_Conversion
)

#-------------------------------------------------------------------------------
# Custom dialect tools
#-------------------------------------------------------------------------------
# Build custom-opt
#-------------------------------------------------------------------------------

add_executable(custom_opt "")
set(_DUMMY_SRC "${CMAKE_CURRENT_BINARY_DIR}/custom_opt_dummy.cc")
file(WRITE ${_DUMMY_SRC} "")
target_sources(custom_opt
  PRIVATE
    ${_DUMMY_SRC}
)

set_target_properties(custom_opt PROPERTIES OUTPUT_NAME custom-opt)

target_link_libraries(custom_opt
  PUBLIC
  "-Wl,--whole-archive"
  dialect
  "-Wl,--no-whole-archive"
  iree_tools_iree_opt_main
)


#-------------------------------------------------------------------------------
# Build custom-translate
#-------------------------------------------------------------------------------

add_executable(custom_translate "")
set(_DUMMY_SRC "${CMAKE_CURRENT_BINARY_DIR}/custom_translate_dummy.cc")
file(WRITE ${_DUMMY_SRC} "")
target_sources(custom_translate
  PRIVATE
    ${_DUMMY_SRC}
)

set_target_properties(custom_translate PROPERTIES OUTPUT_NAME custom-translate)

target_link_libraries(custom_translate
  PUBLIC
  "-Wl,--whole-archive"
  dialect
  iree_compiler_Translation_IREEVM
  "-Wl,--no-whole-archive"
  iree_tools_iree_translate_main
)


#-------------------------------------------------------------------------------
# Further targets
#-------------------------------------------------------------------------------
# Generating declarations and definitions via `mlir-tblgen`
#-------------------------------------------------------------------------------

set(MLIR_TABLEGEN_EXE mlir-tblgen)

set(LLVM_TARGET_DEFINITIONS "custom_ops.td")
set(_TBLGEN_INCLUDE_DIRS ${iree_SOURCE_DIR})
list(APPEND _TBLGEN_INCLUDE_DIRS
  ${iree_SOURCE_DIR}/third_party/llvm-project/mlir/include
)
list(TRANSFORM _TBLGEN_INCLUDE_DIRS PREPEND "-I")

tablegen("MLIR" custom_ops.h.inc  "-gen-op-decls" ${_TBLGEN_INCLUDE_DIRS})
tablegen("MLIR" custom_ops.cc.inc "-gen-op-defs" ${_TBLGEN_INCLUDE_DIRS})

add_custom_target(custom_ops_gen_target DEPENDS custom_ops.h.inc custom_ops.cc.inc)
set_target_properties(custom_ops_gen_target PROPERTIES FOLDER "Tablegenning")

add_library(custom_ops_gen INTERFACE)
add_dependencies(custom_ops_gen custom_ops_gen_target)


#-------------------------------------------------------------------------------
# Embedd the MLIR file into a cc file via `generate_cc_embed_data`.
#-------------------------------------------------------------------------------

# Define arguments passed to generate_cc_embed_data
set(_ARGS)
list(APPEND _ARGS "--output_header=custom.imports.h")
list(APPEND _ARGS "--output_impl=custom.imports.cc")
list(APPEND _ARGS "--identifier=custom_imports")
list(APPEND _ARGS "--cpp_namespace=mlir::iree_compiler::IREE::Custom")
list(APPEND _ARGS "--flatten")
list(APPEND _ARGS "${CMAKE_CURRENT_SOURCE_DIR}/custom.imports.mlir")

# Embed the MLIR file into cc source file
add_custom_command(
  OUTPUT
    "custom.imports.h"
    "custom.imports.cc"
  COMMAND generate_cc_embed_data ${_ARGS}
  DEPENDS generate_cc_embed_data custom.imports.mlir
)


#-------------------------------------------------------------------------------
# Create a library containing the embeded mlir file.
#-------------------------------------------------------------------------------

add_library(custom_imports STATIC "")
target_sources(custom_imports
  PRIVATE
    custom.imports.cc
    custom.imports.h
)