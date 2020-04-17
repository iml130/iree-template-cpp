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

# Define arguments passed to iree-translate
set(_ARGS)
list(APPEND _ARGS "-iree-mlir-to-vm-bytecode-module")
list(APPEND _ARGS "${CMAKE_CURRENT_SOURCE_DIR}/custom_modules_test.mlir")
list(APPEND _ARGS "-o")
list(APPEND _ARGS "custom_modules_test_module.module")

# Translate MLIR file to VM bytecode module
add_custom_command(
  OUTPUT "custom_modules_test_module.module"
  COMMAND ${_TRANSLATE_TOOL_EXECUTABLE} ${_ARGS}
  DEPENDS custom_translate
)


#-------------------------------------------------------------------------------
# Embedd the VM bytcode module into a cc file via `generate_cc_embed_data`.
#-------------------------------------------------------------------------------

# Define arguments passed to generate_cc_embed_data
set(_ARGS)
list(APPEND _ARGS "--output_header=custom_modules_test_module.h")
list(APPEND _ARGS "--output_impl=custom_modules_test_module.cc")
list(APPEND _ARGS "--identifier=custom_modules_test_module")
list(APPEND _ARGS "--cpp_namespace=iree::samples::custom_modules")
list(APPEND _ARGS "--flatten")
list(APPEND _ARGS "custom_modules_test_module.module")

# Embed VM bytecode module into cc source file
add_custom_command(
  OUTPUT
    "custom_modules_test_module.h"
    "custom_modules_test_module.cc"
  COMMAND generate_cc_embed_data ${_ARGS}
  DEPENDS generate_cc_embed_data custom_modules_test_module.module
)


#-------------------------------------------------------------------------------
# Create a library and thus a CMake target.
#-------------------------------------------------------------------------------

add_library(custom_modules_test_module_cc STATIC "")
target_sources(custom_modules_test_module_cc
  PRIVATE
    custom_modules_test_module.cc
    custom_modules_test_module.h
)


#-------------------------------------------------------------------------------
# Build a excutable for testing.
#-------------------------------------------------------------------------------

add_executable(custom_modules_test "")
target_sources(custom_modules_test
  PRIVATE
    custom_modules_test.cc
)

set_target_properties(custom_modules_test PROPERTIES OUTPUT_NAME custom_modules_test)

target_include_directories(custom_modules_test
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${iree_SOURCE_DIR}
    ${iree_SOURCE_DIR}/third_party/abseil-cpp/
)

target_link_libraries(custom_modules_test
  custom_modules_test_module_cc
  native_module
  absl_core_headers
  absl_strings
  iree_base_api
  iree_base_logging
  iree_hal_api
  "-Wl,--whole-archive"
  iree_hal_vmla_vmla_driver_module
  "-Wl,--no-whole-archive"
  iree_modules_hal_hal
  iree_testing_gtest_main
  iree_vm_vm
  iree_vm_bytecode_module
  iree_vm_ref
  gmock
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
    ${iree_SOURCE_DIR}
)

target_link_libraries(native_module
  iree_base_api
  iree_base_api_util
  iree_base_buffer_string_util
  iree_base_shape
  iree_base_shaped_buffer_string_util
  iree_hal_api
  iree_modules_hal_hal
  iree_vm_vm
  iree_vm_module_abi_cc
)
