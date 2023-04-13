# Licensed under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#-------------------------------------------------------------------------------
# Use `iree-compile` to transform an MLIR file into an VM bytcode module.
#-------------------------------------------------------------------------------

# Resolve the executable binary path for iree-compile from the target name.
set(_TRANSLATE_TOOL_EXECUTABLE $<TARGET_FILE:iree-compile>)

# Define arguments passed to iree-compile
set(_ARGS)
list(APPEND _ARGS "-iree-input-type=mhlo")
list(APPEND _ARGS "--output-format=vm-bytecode")
list(APPEND _ARGS "-iree-hal-target-backends=vmvx")
# Uncomment the line below to use vulkan-spirv backend
#list(APPEND _ARGS "-iree-hal-target-backends=vulkan-spirv")
list(APPEND _ARGS "${CMAKE_CURRENT_SOURCE_DIR}/simple_embedding_test.mlir")
list(APPEND _ARGS "-o")
list(APPEND _ARGS "simple_embedding_test_bytecode_module.vmfb")

# Translate MLIR file to VM bytecode module
add_custom_command(
  OUTPUT "simple_embedding_test_bytecode_module.vmfb"
  COMMAND ${_TRANSLATE_TOOL_EXECUTABLE} ${_ARGS}
  DEPENDS iree-compile
)


#-------------------------------------------------------------------------------
# Embedd the VM bytcode module into a c file via `generate_embed_data`.
#-------------------------------------------------------------------------------

# Define arguments passed to generate_embed_data
set(_ARGS)
list(APPEND _ARGS "--output_header=simple_embedding_test_bytecode_module.h")
list(APPEND _ARGS "--output_impl=simple_embedding_test_bytecode_module.c")
list(APPEND _ARGS "--identifier=simple_embedding_test_bytecode_module")
list(APPEND _ARGS "--flatten")
list(APPEND _ARGS "simple_embedding_test_bytecode_module.vmfb")

# Embed VM bytecode module into c source file
add_custom_command(
  OUTPUT
    "simple_embedding_test_bytecode_module.h"
    "simple_embedding_test_bytecode_module.c"
  COMMAND generate_embed_data ${_ARGS}
  DEPENDS generate_embed_data simple_embedding_test_bytecode_module.vmfb
)


#-------------------------------------------------------------------------------
# Create a library and thus a CMake target.
#-------------------------------------------------------------------------------

add_library(simple_embedding_test_bytecode_module_c STATIC "")
target_sources(simple_embedding_test_bytecode_module_c
  PRIVATE
    simple_embedding_test_bytecode_module.c
    simple_embedding_test_bytecode_module.h
)


#-------------------------------------------------------------------------------
# Build the excutable.
#-------------------------------------------------------------------------------

add_executable(simple_embedding "")
target_sources(simple_embedding
  PRIVATE
    simple_embedding.c
    device_vmvx.c
)

set_target_properties(simple_embedding PROPERTIES OUTPUT_NAME simple_embedding)

target_include_directories(simple_embedding
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(simple_embedding
  simple_embedding_test_bytecode_module_c
  iree_base_base
  iree_hal_hal
  iree_hal_drivers_local_task_registration_registration
  iree_hal_local_loaders_vmvx_module_loader
  iree_modules_hal_hal
  iree_vm_vm
  iree_vm_bytecode_module
)
