// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Forked from IREE (modified to use VMVX).

// A example of setting up the the vmvx driver.

#include "iree/base/api.h"
#include "iree/hal/api.h"
#include "iree/hal/drivers/local_task/registration/driver_module.h"

// Compiled module embedded here to avoid file IO:
#include "simple_embedding_test_bytecode_module.h"

iree_status_t create_sample_device(iree_allocator_t host_allocator,
                                   iree_hal_device_t** out_device) {
  // Only register the local-task HAL driver.
  IREE_RETURN_IF_ERROR(iree_hal_local_task_driver_module_register(
      iree_hal_driver_registry_default()));

  // Create the HAL driver from the name.
  iree_hal_driver_t* driver = NULL;
  iree_string_view_t identifier = iree_make_cstring_view("local-task");
  iree_status_t status = iree_hal_driver_registry_try_create(
      iree_hal_driver_registry_default(), identifier, host_allocator, &driver);

  if (iree_status_is_ok(status)) {
    status = iree_hal_driver_create_default_device(driver, host_allocator,
                                                   out_device);
  }

  iree_hal_driver_release(driver);
  return iree_ok_status();
}

const iree_const_byte_span_t load_bytecode_module_data() {
  const struct iree_file_toc_t* module_file_toc =
      simple_embedding_test_bytecode_module_create();
  return iree_make_const_byte_span(module_file_toc->data,
                                   module_file_toc->size);
}
