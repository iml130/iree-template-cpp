// Copyright 2019 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Forked from IREE with modified includes and module name.

// Tests that our bytecode module can call through into our native module.

#include "custom_modules_test_module.h"
#include "native_module.h"

#include "iree/base/api.h"
#include "iree/hal/api.h"
#include "iree/hal/vmvx/registration/driver_module.h"
#include "iree/modules/hal/module.h"
#include "iree/testing/gtest.h"
#include "iree/testing/status_matchers.h"
#include "iree/vm/api.h"
#include "iree/vm/bytecode_module.h"
#include "iree/vm/ref_cc.h"

namespace {

class CustomModulesTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    IREE_CHECK_OK(iree_hal_vmvx_driver_module_register(
        iree_hal_driver_registry_default()));
  }

  virtual void SetUp() {
    IREE_CHECK_OK(iree_vm_instance_create(iree_allocator_system(), &instance_));

    // TODO(benvanik): move to instance-based registration.
    IREE_CHECK_OK(iree_hal_module_register_types());
    // TODO(benvanik): make a 'don't care' helper method.
    iree_hal_driver_t* hal_driver = nullptr;
    IREE_CHECK_OK(iree_hal_driver_registry_try_create_by_name(
        iree_hal_driver_registry_default(), iree_make_cstring_view("vmvx"),
        iree_allocator_system(), &hal_driver));
    iree_hal_device_t* hal_device = nullptr;
    IREE_CHECK_OK(iree_hal_driver_create_default_device(
        hal_driver, iree_allocator_system(), &hal_device));
    IREE_CHECK_OK(iree_hal_module_create(hal_device, iree_allocator_system(),
                                         &hal_module_));
    hal_allocator_ = iree_hal_device_allocator(hal_device);
    iree_hal_device_release(hal_device);
    iree_hal_driver_release(hal_driver);

    IREE_CHECK_OK(iree_custom_native_module_register_types());
    IREE_CHECK_OK(iree_custom_native_module_create(
        iree_allocator_system(), hal_allocator_, &native_module_));

    const auto* module_file_toc =
        custom_modules_test_module_create();
    IREE_CHECK_OK(iree_vm_bytecode_module_create(
        iree_const_byte_span_t{
            reinterpret_cast<const uint8_t*>(module_file_toc->data),
            module_file_toc->size},
        iree_allocator_null(), iree_allocator_system(), &bytecode_module_));

    std::vector<iree_vm_module_t*> modules = {hal_module_, native_module_,
                                              bytecode_module_};
    IREE_CHECK_OK(iree_vm_context_create_with_modules(
        instance_, IREE_VM_CONTEXT_FLAG_NONE, modules.data(), modules.size(),
        iree_allocator_system(), &context_));
  }

  virtual void TearDown() {
    iree_vm_module_release(hal_module_);
    iree_vm_module_release(native_module_);
    iree_vm_module_release(bytecode_module_);
    iree_vm_context_release(context_);
    iree_vm_instance_release(instance_);
  }

  iree_vm_function_t LookupFunction(const char* function_name) {
    iree_vm_function_t function;
    IREE_CHECK_OK(bytecode_module_->lookup_function(
        bytecode_module_->self, IREE_VM_FUNCTION_LINKAGE_EXPORT,
        iree_make_cstring_view(function_name), &function));
    return function;
  }

  iree_vm_instance_t* instance_ = nullptr;
  iree_vm_context_t* context_ = nullptr;
  iree_vm_module_t* bytecode_module_ = nullptr;
  iree_vm_module_t* native_module_ = nullptr;
  iree_vm_module_t* hal_module_ = nullptr;
  iree_hal_allocator_t* hal_allocator_ = nullptr;
};

TEST_F(CustomModulesTest, ReverseAndPrint) {
  // Allocate one of our custom message types to pass in.
  iree_custom_message_t* input_message = nullptr;
  IREE_ASSERT_OK(
      iree_custom_message_wrap(iree_make_cstring_view("hello world!"),
                               iree_allocator_system(), &input_message));
  iree_vm_value_t count = iree_vm_value_make_i32(5);

  // Pass in the message and number of times to print it.
  // TODO(benvanik): make a macro/magic.
  iree::vm::ref<iree_vm_list_t> inputs;
  IREE_ASSERT_OK(iree_vm_list_create(/*element_type=*/nullptr, 2,
                                     iree_allocator_system(), &inputs));
  iree_vm_ref_t input_message_ref = iree_custom_message_move_ref(input_message);
  IREE_ASSERT_OK(iree_vm_list_push_ref_move(inputs.get(), &input_message_ref));
  IREE_ASSERT_OK(iree_vm_list_push_value(inputs.get(), &count));

  // Prepare outputs list to accept the results from the invocation.
  iree::vm::ref<iree_vm_list_t> outputs;
  IREE_ASSERT_OK(iree_vm_list_create(/*element_type=*/nullptr, 1,
                                     iree_allocator_system(), &outputs));

  // Synchronously invoke the function.
  IREE_ASSERT_OK(iree_vm_invoke(context_, LookupFunction("reverseAndPrint"),
                                IREE_VM_INVOCATION_FLAG_NONE,
                                /*policy=*/nullptr, inputs.get(), outputs.get(),
                                iree_allocator_system()));

  // Read back the message that we reversed inside of the module.
  iree_custom_message_t* reversed_message =
      (iree_custom_message_t*)iree_vm_list_get_ref_deref(
          outputs.get(), 0, iree_custom_message_get_descriptor());
  ASSERT_NE(nullptr, reversed_message);
  char result_buffer[256];
  IREE_ASSERT_OK(iree_custom_message_read_value(reversed_message, result_buffer,
                                                IREE_ARRAYSIZE(result_buffer)));
  EXPECT_STREQ("!dlrow olleh", result_buffer);
}

TEST_F(CustomModulesTest, PrintTensor) {
  // Allocate the buffer we'll be printing.
  static iree_hal_dim_t kShape[] = {2, 4};
  static const float kBufferContents[2 * 4] = {0.0f, 1.0f, 2.0f, 3.0f,
                                               4.0f, 5.0f, 6.0f, 7.0f};
  iree_hal_buffer_params_t params = {0};
  params.type =
      IREE_HAL_MEMORY_TYPE_HOST_LOCAL | IREE_HAL_MEMORY_TYPE_DEVICE_VISIBLE;
  params.usage = IREE_HAL_BUFFER_USAGE_DISPATCH |
                 IREE_HAL_BUFFER_USAGE_TRANSFER | IREE_HAL_BUFFER_USAGE_MAPPING;
  iree_hal_buffer_view_t* buffer_view = nullptr;
  IREE_ASSERT_OK(iree_hal_buffer_view_allocate_buffer(
      hal_allocator_, kShape, IREE_ARRAYSIZE(kShape),
      IREE_HAL_ELEMENT_TYPE_FLOAT_32, IREE_HAL_ENCODING_TYPE_DENSE_ROW_MAJOR,
      params,
      iree_make_const_byte_span(kBufferContents, sizeof(kBufferContents)),
      &buffer_view));

  // Pass in the tensor as an expanded HAL buffer.
  iree::vm::ref<iree_vm_list_t> inputs;
  IREE_ASSERT_OK(iree_vm_list_create(/*element_type=*/nullptr, 1,
                                     iree_allocator_system(), &inputs));
  iree_vm_ref_t input_buffer_view_ref =
      iree_hal_buffer_view_move_ref(buffer_view);
  IREE_ASSERT_OK(
      iree_vm_list_push_ref_move(inputs.get(), &input_buffer_view_ref));

  // Prepare outputs list to accept the results from the invocation.
  iree::vm::ref<iree_vm_list_t> outputs;
  IREE_ASSERT_OK(iree_vm_list_create(/*element_type=*/nullptr, 1,
                                     iree_allocator_system(), &outputs));

  // Synchronously invoke the function.
  IREE_ASSERT_OK(iree_vm_invoke(context_, LookupFunction("printTensor"),
                                IREE_VM_INVOCATION_FLAG_NONE,
                                /*policy=*/nullptr, inputs.get(), outputs.get(),
                                iree_allocator_system()));

  // Read back the message that we printed inside of the module.
  iree_custom_message_t* printed_message =
      (iree_custom_message_t*)iree_vm_list_get_ref_deref(
          outputs.get(), 0, iree_custom_message_get_descriptor());
  ASSERT_NE(nullptr, printed_message);
  char result_buffer[256];
  IREE_ASSERT_OK(iree_custom_message_read_value(printed_message, result_buffer,
                                                IREE_ARRAYSIZE(result_buffer)));
  EXPECT_STREQ("2x4xf32=[0 1 2 3][4 5 6 7]", result_buffer);
}

TEST_F(CustomModulesTest, RoundTripTensor) {
  // Allocate the buffer we'll be printing/parsing.
  static iree_hal_dim_t kShape[] = {2, 4};
  static const float kBufferContents[2 * 4] = {0.0f, 1.0f, 2.0f, 3.0f,
                                               4.0f, 5.0f, 6.0f, 7.0f};
  iree_hal_buffer_params_t params = {0};
  params.type =
      IREE_HAL_MEMORY_TYPE_HOST_LOCAL | IREE_HAL_MEMORY_TYPE_DEVICE_VISIBLE;
  params.usage = IREE_HAL_BUFFER_USAGE_DISPATCH |
                 IREE_HAL_BUFFER_USAGE_TRANSFER | IREE_HAL_BUFFER_USAGE_MAPPING;
  iree_hal_buffer_view_t* buffer_view = nullptr;
  IREE_ASSERT_OK(iree_hal_buffer_view_allocate_buffer(
      hal_allocator_, kShape, IREE_ARRAYSIZE(kShape),
      IREE_HAL_ELEMENT_TYPE_FLOAT_32, IREE_HAL_ENCODING_TYPE_DENSE_ROW_MAJOR,
      params,
      iree_make_const_byte_span(kBufferContents, sizeof(kBufferContents)),
      &buffer_view));

  // Pass in the tensor as an expanded HAL buffer.
  iree::vm::ref<iree_vm_list_t> inputs;
  IREE_ASSERT_OK(iree_vm_list_create(/*element_type=*/nullptr, 1,
                                     iree_allocator_system(), &inputs));
  iree_vm_ref_t input_buffer_view_ref =
      iree_hal_buffer_view_move_ref(buffer_view);
  IREE_ASSERT_OK(
      iree_vm_list_push_ref_move(inputs.get(), &input_buffer_view_ref));

  // Prepare outputs list to accept the results from the invocation.
  iree::vm::ref<iree_vm_list_t> outputs;
  IREE_ASSERT_OK(iree_vm_list_create(/*element_type=*/nullptr, 1,
                                     iree_allocator_system(), &outputs));

  // Synchronously invoke the function.
  IREE_ASSERT_OK(iree_vm_invoke(context_, LookupFunction("roundTripTensor"),
                                IREE_VM_INVOCATION_FLAG_NONE,
                                /*policy=*/nullptr, inputs.get(), outputs.get(),
                                iree_allocator_system()));

  // Read back the message that's been moved around.
  iree_custom_message_t* printed_message =
      (iree_custom_message_t*)iree_vm_list_get_ref_deref(
          outputs.get(), 0, iree_custom_message_get_descriptor());
  ASSERT_NE(nullptr, printed_message);
  char result_buffer[256];
  IREE_ASSERT_OK(iree_custom_message_read_value(printed_message, result_buffer,
                                                IREE_ARRAYSIZE(result_buffer)));
  EXPECT_STREQ("2x4xf32=[0 1 2 3][4 5 6 7]", result_buffer);
}

}  // namespace
