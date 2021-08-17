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
  MLIRSideEffectInterfaces
  MLIRTransforms
  iree_compiler_Dialect_HAL_Conversion_Conversion
  iree_compiler_Dialect_HAL_IR_IR
  iree_compiler_Dialect_Util_IR_IR
  iree_compiler_Dialect_VM_Conversion_Conversion
)

# NOTE: IREE disables RTTI by default. See the main CMakeLists.txt.
target_compile_options(dialect PRIVATE "-fno-rtti")

#-------------------------------------------------------------------------------
# Custom dialect tools
#-------------------------------------------------------------------------------
# Build custom-opt
#-------------------------------------------------------------------------------

add_executable(custom_opt "")
target_sources(custom_opt
  PRIVATE
    custom-opt-main.cc
    init_dialect.h
)

set_target_properties(custom_opt PROPERTIES OUTPUT_NAME custom-opt)

target_link_libraries(custom_opt
  PUBLIC
  dialect
  iree_tools_init_passes_and_dialects
  iree_tools_init_targets
  LLVMSupport
  MLIRIR
  MLIROptLib
  MLIRSupport
)


#-------------------------------------------------------------------------------
# Build custom-translate
#-------------------------------------------------------------------------------

add_executable(custom_translate "")
target_sources(custom_translate
  PRIVATE
    custom-translate-main.cc
    init_dialect.h
)

set_target_properties(custom_translate PROPERTIES OUTPUT_NAME custom-translate)

target_link_libraries(custom_translate
  PUBLIC
  dialect
  iree_tools_init_compiler_modules
  iree_tools_init_iree_passes_and_dialects
  iree_tools_init_mlir_passes_and_dialects
  iree_tools_init_targets
  iree_tools_init_translations
  iree_tools_init_xla_dialects
  LLVMSupport
  MLIRIR
  MLIRPass
  MLIRSCFTransforms
  MLIRSupport
  MLIRTranslation
  iree_compiler_Dialect_VM_Target_Bytecode_Bytecode
  iree_compiler_Dialect_VM_Target_init_targets
  iree_compiler_Translation_IREEVM
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
# Embedd the MLIR file into a c file via `generate_embed_data`.
#-------------------------------------------------------------------------------

# Define arguments passed to generate_embed_data
set(_ARGS)
list(APPEND _ARGS "--output_header=custom.imports.h")
list(APPEND _ARGS "--output_impl=custom.imports.c")
list(APPEND _ARGS "--identifier=custom_imports")
list(APPEND _ARGS "--flatten")
list(APPEND _ARGS "${CMAKE_CURRENT_SOURCE_DIR}/custom.imports.mlir")

# Embed the MLIR file into c source file
add_custom_command(
  OUTPUT
    "custom.imports.h"
    "custom.imports.c"
  COMMAND generate_embed_data ${_ARGS}
  DEPENDS generate_embed_data custom.imports.mlir
)


#-------------------------------------------------------------------------------
# Create a library containing the embeded mlir file.
#-------------------------------------------------------------------------------

add_library(custom_imports STATIC "")
target_sources(custom_imports
  PRIVATE
    custom.imports.c
    custom.imports.h
)
