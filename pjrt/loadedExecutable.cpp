#include "context.hpp"
#include "detail/callbackUserData.hpp"
#include "deviceView.hpp"
#include "event.hpp"
#include "executable.hpp"
#include "loadedExecutable.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif

// Assume pjrt_c_api.h is in the same directory or an include path
#include "pjrt_c_api.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

  #include <cassert>
#include <stdexcept>
#include <vector>

namespace pjrt {

LoadedExecutable::LoadedExecutable(const Context &context, PJRT_LoadedExecutable *loadedExecutable) : context_(context), loadedExecutable_(loadedExecutable) {}

LoadedExecutable::LoadedExecutable(LoadedExecutable &&other) : context_(other.context_), loadedExecutable_(other.loadedExecutable_) {
  other.loadedExecutable_ = nullptr;
}

LoadedExecutable& LoadedExecutable::operator=(LoadedExecutable &&other) {
  assert(((void)"Cannot assign a LoadedExecutable from one context to another", &other.context_ == &context_));
  loadedExecutable_ = other.loadedExecutable_;
  other.loadedExecutable_ = nullptr;
  return *this;
}

LoadedExecutable::~LoadedExecutable() {
  if (loadedExecutable_ == nullptr) {
    return;
  }
  PJRT_LoadedExecutable_Destroy_Args exec_destroy_args;
  exec_destroy_args.struct_size = PJRT_LoadedExecutable_Destroy_Args_STRUCT_SIZE;
  exec_destroy_args.extension_start = nullptr;
  exec_destroy_args.executable = loadedExecutable_;
  PJRT_Error* exec_destroy_error = context_.pjrtApi_->PJRT_LoadedExecutable_Destroy(&exec_destroy_args);
  if (exec_destroy_error != nullptr) {
    const pjrt::Exception ex = context_.convertPjrtErrorToException(exec_destroy_error, "PJRT_LoadedExecutable_Destroy", __FILE__, __LINE__);
    std::cerr << "pjrt::LoadedExecutable destructor failed to destroy PJRT_LoadedExecutable: \"" << ex.what() << "\"" << std::endl;
  }
}

void LoadedExecutable::destroy() {
  if (loadedExecutable_ == nullptr) {
    return;
  }
  PJRT_LoadedExecutable_Destroy_Args exec_destroy_args;
  exec_destroy_args.struct_size = PJRT_LoadedExecutable_Destroy_Args_STRUCT_SIZE;
  exec_destroy_args.extension_start = nullptr;
  exec_destroy_args.executable = loadedExecutable_;
  PJRT_Error* exec_destroy_error = context_.pjrtApi_->PJRT_LoadedExecutable_Destroy(&exec_destroy_args);
  if (exec_destroy_error != nullptr) {
    throw context_.convertPjrtErrorToException(exec_destroy_error, "PJRT_LoadedExecutable_Destroy", __FILE__, __LINE__);
  }
}

std::future<std::vector<Buffer>> LoadedExecutable::execute(
    const DeviceView& device, std::vector<Buffer*>& argument_handles) {
  // Prepare and Execute the Compiled Program
  PJRT_ExecuteOptions exec_options;
  exec_options.struct_size = PJRT_ExecuteOptions_STRUCT_SIZE;
  exec_options.extension_start = nullptr;
  exec_options.launch_id = 0; // Default launch ID
  exec_options.num_send_ops = 0;
  exec_options.send_callbacks = nullptr;
  exec_options.num_recv_ops = 0;
  exec_options.recv_callbacks = nullptr;
  exec_options.non_donatable_input_indices = nullptr;
  exec_options.num_non_donatable_input_indices = 0;
  exec_options.context = nullptr;

  PJRT_LoadedExecutable_Execute_Args exec_args;
  exec_args.struct_size = PJRT_LoadedExecutable_Execute_Args_STRUCT_SIZE;
  exec_args.extension_start = nullptr;
  exec_args.executable = loadedExecutable_;
  exec_args.options = &exec_options;

  // Argument lists: Our program takes multiple arguments. We are executing on 1 device.
  std::vector<PJRT_Buffer*> actual_input_buffers_for_device0(argument_handles.size());
  for (size_t i=0; i<argument_handles.size(); ++i) {
    actual_input_buffers_for_device0[i] = argument_handles[i]->c_buffer();
  }
  PJRT_Buffer* const* argument_lists_for_all_devices[] = {
      actual_input_buffers_for_device0.data()
  };
  exec_args.argument_lists = argument_lists_for_all_devices;
  exec_args.num_devices = 1; // We are launching on a single device instance here
  exec_args.num_args = argument_handles.size();

    // Output setup
  const Executable executable = getExecutable();
  std::vector<PJRT_Buffer*> raw_output_c_buffers(executable.getNumOutputs());
  PJRT_Buffer** output_list_for_device0[1];
  output_list_for_device0[0] = raw_output_c_buffers.data();
  exec_args.output_lists = output_list_for_device0;

  // Device complete events: one event per device in the launch
  PJRT_Event* device_complete_event_handles[1];
  exec_args.device_complete_events = device_complete_event_handles;
  exec_args.execute_device = device.device_; // Execute on the specific device we chose

  PJRT_Error* exec_error = context_.pjrtApi_->PJRT_LoadedExecutable_Execute(&exec_args);
  if (exec_error != nullptr) {
    // TODO: We assume that there are no buffers or events that we are responsible for cleaning up. The PJRT API documentation is unclear in this case.
    throw context_.convertPjrtErrorToException(exec_error, "PJRT_LoadedExecutable_Execute", __FILE__, __LINE__);
  }

  const std::vector<std::vector<int64_t>> outputDimensions = executable.getOutputDimensions();
  std::vector<Buffer> final_output_buffers;
  final_output_buffers.reserve(executable.getNumOutputs());
  for (size_t i = 0; i < executable.getNumOutputs(); ++i) {
    final_output_buffers.emplace_back(context_, raw_output_c_buffers[i], std::move(outputDimensions[i]));
  }

  // Create CallbackUserData with the fully formed Buffer
  std::unique_ptr<detail::CallbackUserData<std::vector<Buffer>>> callbackUserData =
      std::make_unique<detail::CallbackUserData<std::vector<Buffer>>>(context_, std::move(final_output_buffers));

  return context_.getFutureForEvent(device_complete_event_handles[0], std::move(callbackUserData));
}

Executable LoadedExecutable::getExecutable() const {
  // Query PJRT for the executable's output shape.
  PJRT_LoadedExecutable_GetExecutable_Args args;
  args.struct_size = PJRT_LoadedExecutable_GetExecutable_Args_STRUCT_SIZE;
  args.loaded_executable = loadedExecutable_;
  PJRT_Error *error = context_.pjrtApi_->PJRT_LoadedExecutable_GetExecutable(&args);
  if (error != nullptr) {
    throw context_.convertPjrtErrorToException(error, "PJRT_LoadedExecutable_GetExecutable", __FILE__, __LINE__);
  }
  return Executable(context_, args.executable);
}

} // namespace pjrt