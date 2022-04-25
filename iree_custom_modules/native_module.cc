// Copyright 2019 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Forked from IREE with modified includes.

#include "native_module.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "iree/base/api.h"
#include "iree/base/status_cc.h"
#include "iree/hal/api.h"
#include "iree/modules/hal/module.h"
#include "iree/vm/native_module_cc.h"
#include "iree/vm/ref_cc.h"

//===----------------------------------------------------------------------===//
// !custom.message type
//===----------------------------------------------------------------------===//

// Runtime type descriptor for the !custom.message describing how to manage it
// and destroy it. The type ID is allocated at runtime and does not need to
// match the compiler ID.
static iree_vm_ref_type_descriptor_t iree_custom_message_descriptor = {0};

// The "message" type we use to store string messages to print.
// This could be arbitrarily complex or simply wrap another user-defined type.
// The descriptor that is registered at startup defines how to manage the
// lifetime of the type (such as which destruction function is called, if any).
// See ref.h for more information and additional utilities.
typedef struct iree_custom_message_t {
  // Ideally first; used to track the reference count of the object.
  iree_vm_ref_object_t ref_object;
  // Allocator the message was created from.
  // Ideally pools and nested allocators would be used to avoid needing to store
  // the allocator with every object.
  iree_allocator_t allocator;
  // String message value.
  iree_string_view_t value;
} iree_custom_message_t;

IREE_VM_DEFINE_TYPE_ADAPTERS(iree_custom_message, iree_custom_message_t);

iree_status_t iree_custom_message_create(iree_string_view_t value,
                                         iree_allocator_t allocator,
                                         iree_custom_message_t** out_message) {
  IREE_ASSERT_ARGUMENT(out_message);
  // Note that we allocate the message and the string value together.
  iree_custom_message_t* message = NULL;
  IREE_RETURN_IF_ERROR(iree_allocator_malloc(
      allocator, sizeof(*message) + value.size, (void**)&message));
  message->ref_object.counter = IREE_ATOMIC_VAR_INIT(1);
  message->allocator = allocator;
  message->value.data = ((const char*)message) + sizeof(iree_custom_message_t);
  message->value.size = value.size;
  memcpy((void*)message->value.data, value.data, message->value.size);
  *out_message = message;
  return iree_ok_status();
}

iree_status_t iree_custom_message_wrap(iree_string_view_t value,
                                       iree_allocator_t allocator,
                                       iree_custom_message_t** out_message) {
  IREE_ASSERT_ARGUMENT(out_message);
  iree_custom_message_t* message = NULL;
  IREE_RETURN_IF_ERROR(
      iree_allocator_malloc(allocator, sizeof(*message), (void**)&message));
  message->ref_object.counter = IREE_ATOMIC_VAR_INIT(1);
  message->allocator = allocator;
  message->value = value;  // Unowned.
  *out_message = message;
  return iree_ok_status();
}

void iree_custom_message_destroy(void* ptr) {
  iree_custom_message_t* message = (iree_custom_message_t*)ptr;
  iree_allocator_free(message->allocator, ptr);
}

iree_status_t iree_custom_message_read_value(iree_custom_message_t* message,
                                             char* buffer,
                                             size_t buffer_capacity) {
  IREE_ASSERT_ARGUMENT(message);
  IREE_ASSERT_ARGUMENT(buffer);
  if (buffer_capacity < message->value.size + 1) {
    // Not an error; just a size query.
    return iree_status_from_code(IREE_STATUS_OUT_OF_RANGE);
  }
  memcpy(buffer, message->value.data, message->value.size);
  buffer[message->value.size] = 0;
  return iree_ok_status();
}

iree_status_t iree_custom_native_module_register_types() {
  if (iree_custom_message_descriptor.type) {
    return iree_ok_status();  // Already registered.
  }
  iree_custom_message_descriptor.type_name =
      iree_make_cstring_view("custom.message");
  iree_custom_message_descriptor.offsetof_counter =
      offsetof(iree_custom_message_t, ref_object.counter);
  iree_custom_message_descriptor.destroy = iree_custom_message_destroy;
  return iree_vm_ref_register_type(&iree_custom_message_descriptor);
}

//===----------------------------------------------------------------------===//
// VM module interface implementation
//===----------------------------------------------------------------------===//

namespace iree {
namespace samples {
namespace {

// Per-context module state.
// This can contain "globals" and other arbitrary state.
//
// Thread-compatible; the runtime will not issue multiple calls at the same
// time using the same state. If the implementation uses external threads then
// it must synchronize itself.
class CustomModuleState final {
 public:
  explicit CustomModuleState(iree_allocator_t host_allocator,
                             vm::ref<iree_hal_allocator_t> device_allocator)
      : host_allocator_(host_allocator),
        device_allocator_(std::move(device_allocator)) {}
  ~CustomModuleState() = default;

  Status Initialize(int32_t unique_id) {
    // Allocate a unique ID to demonstrate per-context state.
    auto str_buffer = "ctx_" + std::to_string(unique_id);
    IREE_RETURN_IF_ERROR(
        iree_custom_message_create(iree_make_cstring_view(str_buffer.c_str()),
                                   host_allocator_, &unique_message_));

    return OkStatus();
  }

  // custom.buffer_to_message(%buffer_view) -> %result
  StatusOr<vm::ref<iree_custom_message_t>> BufferToMessage(
      vm::ref<iree_hal_buffer_view_t> buffer_view) {
    // Convert the buffer view to a [shape]x[type]=[contents] string.
    std::string string_value(4096, '\0');
    iree_status_t status;
    do {
      iree_host_size_t actual_length = 0;
      status = iree_hal_buffer_view_format(
          buffer_view.get(), /*max_element_count=*/1024,
          string_value.size() + 1, &string_value[0], &actual_length);
      string_value.resize(actual_length);
    } while (iree_status_is_out_of_range(status));
    IREE_RETURN_IF_ERROR(std::move(status));

    // Pack the string contents into a message.
    vm::ref<iree_custom_message_t> message;
    IREE_RETURN_IF_ERROR(iree_custom_message_create(
        iree_string_view_t{string_value.data(), string_value.size()},
        host_allocator_, &message));
    return std::move(message);
  }

  // custom.message_to_buffer(%message) -> %buffer_view
  StatusOr<vm::ref<iree_hal_buffer_view_t>> MessageToBuffer(
      vm::ref<iree_custom_message_t> message) {
    // Convert the [shape]x[type]=[contents] string to a buffer view.
    vm::ref<iree_hal_buffer_view_t> buffer_view;
    IREE_RETURN_IF_ERROR(
        iree_hal_buffer_view_parse(message->value, device_allocator_.get(),
                                   &buffer_view),
        "parsing value '%.*s'", (int)message->value.size, message->value.data);
    return std::move(buffer_view);
  }

  // custom.print(%message, %count)
  Status Print(vm::ref<iree_custom_message_t> message, int32_t count) {
    for (int i = 0; i < count; ++i) {
      fwrite(message->value.data, 1, message->value.size, stdout);
      fputc('\n', stdout);
    }
    fflush(stdout);
    return OkStatus();
  }

  // custom.reverse(%message) -> %result
  StatusOr<vm::ref<iree_custom_message_t>> Reverse(
      vm::ref<iree_custom_message_t> message) {
    vm::ref<iree_custom_message_t> reversed_message;
    IREE_RETURN_IF_ERROR(iree_custom_message_create(
        message->value, message->allocator, &reversed_message));
    char* str_ptr = const_cast<char*>(reversed_message->value.data);
    for (int low = 0, high = reversed_message->value.size - 1; low < high;
         ++low, --high) {
      char temp = str_ptr[low];
      str_ptr[low] = str_ptr[high];
      str_ptr[high] = temp;
    }
    return std::move(reversed_message);
  }

  // custom.get_unique_message() -> %result
  StatusOr<vm::ref<iree_custom_message_t>> GetUniqueMessage() {
    return vm::retain_ref(unique_message_);
  }

 private:
  // Allocator that the caller requested we use for any allocations we need to
  // perform during operation.
  iree_allocator_t host_allocator_ = iree_allocator_system();

  // HAL buffer allocator that uses host-local memory. This is just for this
  // test as we don't actually use a HAL device and don't have a real device
  // allocator to use.
  vm::ref<iree_hal_allocator_t> device_allocator_;

  // A unique message owned by the state and returned to the VM.
  // This demonstrates any arbitrary per-context state one may want to store.
  vm::ref<iree_custom_message_t> unique_message_;
};

// Function table mapping imported function names to their implementation.
// The signature of the target function is expected to match that in the
// custom.imports.mlir file.
static const vm::NativeFunction<CustomModuleState> kCustomModuleFunctions[] = {
    vm::MakeNativeFunction("buffer_to_message",
                           &CustomModuleState::BufferToMessage),
    vm::MakeNativeFunction("message_to_buffer",
                           &CustomModuleState::MessageToBuffer),
    vm::MakeNativeFunction("print", &CustomModuleState::Print),
    vm::MakeNativeFunction("reverse", &CustomModuleState::Reverse),
    vm::MakeNativeFunction("get_unique_message",
                           &CustomModuleState::GetUniqueMessage),
};

// The module instance that will be allocated and reused across contexts.
// Any context-specific state must be stored in a state structure such as
// CustomModuleState below.
//
// Assumed thread-safe (by construction here, as it's immutable), though if more
// state is stored here it will need to be synchronized by the implementation.
class CustomModule final : public vm::NativeModule<CustomModuleState> {
 public:
  using vm::NativeModule<CustomModuleState>::NativeModule;

  // Example of global initialization (shared across all contexts), such as
  // loading native libraries or creating shared pools.
  Status Initialize(iree_hal_allocator_t* device_allocator) {
    device_allocator_ = vm::retain_ref(device_allocator);
    next_unique_id_ = 0;
    return OkStatus();
  }

  // Creates per-context state when the module is added to a new context.
  // May be called from any thread.
  StatusOr<std::unique_ptr<CustomModuleState>> CreateState(
      iree_allocator_t host_allocator) override {
    auto state = std::make_unique<CustomModuleState>(
        host_allocator, vm::retain_ref(device_allocator_));
    IREE_RETURN_IF_ERROR(state->Initialize(next_unique_id_++));
    return state;
  }

 private:
  // Used for any HAL buffer allocations we need to make.
  vm::ref<iree_hal_allocator_t> device_allocator_;

  // The next ID to allocate for states that will be used to form the
  // per-context unique message. This shows state at the module level. Note that
  // this must be thread-safe.
  std::atomic<int32_t> next_unique_id_;
};

}  // namespace

// Note that while we are using C++ bindings internally we still expose the
// module as a C instance. This hides the details of our implementation.
extern "C" iree_status_t iree_custom_native_module_create(
    iree_allocator_t host_allocator, iree_hal_allocator_t* device_allocator,
    iree_vm_module_t** out_module) {
  IREE_ASSERT_ARGUMENT(out_module);
  *out_module = NULL;
  auto module = std::make_unique<CustomModule>(
      "custom", host_allocator,
      iree::span<const vm::NativeFunction<CustomModuleState>>(
          kCustomModuleFunctions));
  IREE_RETURN_IF_ERROR(module->Initialize(device_allocator));
  *out_module = module.release()->interface();
  return iree_ok_status();
}

}  // namespace samples
}  // namespace iree
