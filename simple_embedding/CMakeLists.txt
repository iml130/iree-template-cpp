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
# Use `iree-translate` to transform an MLIR file into an VM bytcode module.
#-------------------------------------------------------------------------------

# Resolve the executable binary path for iree-translate from the target name.
set(_TRANSLATE_TOOL_EXECUTABLE $<TARGET_FILE:iree_tools_iree-translate>)

# Define arguments passed to iree-translate
set(_ARGS)
list(APPEND _ARGS "-iree-mlir-to-vm-bytecode-module")
list(APPEND _ARGS "-iree-hal-target-backends=vmla")
list(APPEND _ARGS "-iree-hal-target-backends=vulkan-spirv")
list(APPEND _ARGS "${CMAKE_CURRENT_SOURCE_DIR}/simple_embedding_test.mlir")
list(APPEND _ARGS "-o")
list(APPEND _ARGS "simple_embedding_test_bytecode_module.module")

# Translate MLIR file to VM bytecode module
add_custom_command(
  OUTPUT "simple_embedding_test_bytecode_module.module"
  COMMAND ${_TRANSLATE_TOOL_EXECUTABLE} ${_ARGS}
  DEPENDS iree_tools_iree-translate
)


#-------------------------------------------------------------------------------
# Embedd the VM bytcode module into a cc file via `generate_cc_embed_data`.
#-------------------------------------------------------------------------------

# Define arguments passed to generate_cc_embed_data
set(_ARGS)
list(APPEND _ARGS "--output_header=simple_embedding_test_bytecode_module.h")
list(APPEND _ARGS "--output_impl=simple_embedding_test_bytecode_module.cc")
list(APPEND _ARGS "--identifier=simple_embedding_test_bytecode_module")
list(APPEND _ARGS "--cpp_namespace=iree::samples")
list(APPEND _ARGS "--flatten")
list(APPEND _ARGS "simple_embedding_test_bytecode_module.module")

# Embed VM bytecode module into cc source file
add_custom_command(
  OUTPUT
    "simple_embedding_test_bytecode_module.h"
    "simple_embedding_test_bytecode_module.cc"
  COMMAND generate_cc_embed_data ${_ARGS}
  DEPENDS generate_cc_embed_data simple_embedding_test_bytecode_module.module
)


#-------------------------------------------------------------------------------
# Create a library and thus a CMake target.
#-------------------------------------------------------------------------------

add_library(simple_embedding_test_bytecode_module_cc STATIC "")
target_sources(simple_embedding_test_bytecode_module_cc
  PRIVATE
    simple_embedding_test_bytecode_module.cc
    simple_embedding_test_bytecode_module.h
)


#-------------------------------------------------------------------------------
# Build a excutable for testing.
#-------------------------------------------------------------------------------

add_executable(simple_embedding_test "")
target_sources(simple_embedding_test
  PRIVATE
    simple_embedding_test.cc
)

set_target_properties(simple_embedding_test PROPERTIES OUTPUT_NAME simple_embedding_test)

target_include_directories(simple_embedding_test
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(simple_embedding_test
  simple_embedding_test_bytecode_module_cc
  absl_core_headers
  absl_strings
  iree_base_api
  iree_base_logging
  iree_hal_api
  "-Wl,--whole-archive"
  iree_hal_vmla_vmla_driver_module
  iree_hal_vulkan_vulkan_driver_module
  "-Wl,--no-whole-archive"
  iree_modules_hal_hal
  iree_testing_gtest
  iree_testing_gtest_main
  iree_vm_vm
  iree_vm_bytecode_module
  iree_vm_ref_cc
)
