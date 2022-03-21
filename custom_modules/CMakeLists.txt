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

add_subdirectory(dialect)

#-------------------------------------------------------------------------------
# Use `custom-translate` to transform an MLIR file into an VM bytcode module.
#-------------------------------------------------------------------------------

# Resolve the executable binary path for custom-translate from the target name.
set(_TRANSLATE_TOOL_EXECUTABLE $<TARGET_FILE:custom_translate>)

# Define arguments passed to iree-compile
set(_ARGS)
list(APPEND _ARGS "-iree-mlir-to-vm-bytecode-module")
list(APPEND _ARGS "-iree-hal-target-backends=vmvx")
list(APPEND _ARGS "${CMAKE_CURRENT_SOURCE_DIR}/custom_modules_test.mlir")
list(APPEND _ARGS "-o")
list(APPEND _ARGS "custom_modules_test_module.vmfb")

# Translate MLIR file to VM bytecode module
add_custom_command(
  OUTPUT "custom_modules_test_module.vmfb"
  COMMAND ${_TRANSLATE_TOOL_EXECUTABLE} ${_ARGS}
  DEPENDS custom_translate
)


#-------------------------------------------------------------------------------
# Embedd the VM bytcode module into a c file via `generate_embed_data`.
#-------------------------------------------------------------------------------

# Define arguments passed to generate_embed_data
set(_ARGS)
list(APPEND _ARGS "--output_header=custom_modules_test_module.h")
list(APPEND _ARGS "--output_impl=custom_modules_test_module.c")
list(APPEND _ARGS "--identifier=custom_modules_test_module")
list(APPEND _ARGS "--flatten")
list(APPEND _ARGS "custom_modules_test_module.vmfb")

# Embed VM bytecode module into c source file
add_custom_command(
  OUTPUT
    "custom_modules_test_module.h"
    "custom_modules_test_module.c"
  COMMAND generate_embed_data ${_ARGS}
  DEPENDS generate_embed_data custom_modules_test_module.vmfb
)


#-------------------------------------------------------------------------------
# Create a library and thus a CMake target.
#-------------------------------------------------------------------------------

add_library(custom_modules_test_module_c STATIC "")
target_sources(custom_modules_test_module_c
  PRIVATE
    custom_modules_test_module.c
    custom_modules_test_module.h
)


#-------------------------------------------------------------------------------
# Build a excutable for testing.
#-------------------------------------------------------------------------------

add_executable(extern_custom_modules_test "")
target_sources(extern_custom_modules_test
  PRIVATE
    custom_modules_test.cc
)

set_target_properties(extern_custom_modules_test PROPERTIES OUTPUT_NAME extern_custom_modules_test)

target_include_directories(extern_custom_modules_test
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${IREE_SOURCE_DIR}
)

target_link_libraries(extern_custom_modules_test
  custom_modules_test_module_c
  native_module
  iree_base_base
  iree_base_logging
  iree_hal_hal
  iree_hal_vmvx_registration_registration
  iree_modules_hal_hal
  iree_testing_gtest
  iree_testing_gtest_main
  iree_vm_vm
  iree_vm_bytecode_module
  iree_vm_cc
)


#-------------------------------------------------------------------------------
# Create the native_module library.
#-------------------------------------------------------------------------------

add_library(native_module STATIC "")
target_sources(native_module
  PRIVATE
    "native_module.h"
    "native_module.cc"
)

target_include_directories(native_module
  PUBLIC
    ${IREE_SOURCE_DIR}
)

target_link_libraries(native_module
  iree_base_base
  iree_base_cc
  iree_hal_hal
  iree_modules_hal_hal
  iree_vm_vm
  iree_vm_cc
)
