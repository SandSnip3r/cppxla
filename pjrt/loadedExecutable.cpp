#include "context.hpp"
#include "detail/callbackUserData.hpp"
#include "deviceView.hpp"
#include "event.hpp"
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

#include <stdexcept>
#include <vector>

namespace pjrt {

LoadedExecutable::LoadedExecutable(const Context &context, PJRT_LoadedExecutable *loadedExecutable) : context_(context), loadedExecutable_(loadedExecutable) {
  // Query PJRT for the executable's output shape.
  PJRT_LoadedExecutable_GetExecutable_Args args;
  args.struct_size = PJRT_LoadedExecutable_GetExecutable_Args_STRUCT_SIZE;
  args.loaded_executable = loadedExecutable_;
  PJRT_Error *error = context_.pjrtApi_->PJRT_LoadedExecutable_GetExecutable(&args);
  if (error != nullptr) {
    throw context_.convertPjrtErrorToException(error, "PJRT_LoadedExecutable_GetExecutable", __FILE__, __LINE__);
  }
  executable_ = Executable(context, args.executable);
}

LoadedExecutable::~LoadedExecutable() {
  if (!loadedExecutable_) {
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
  if (!loadedExecutable_) {
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

std::future<Buffer> LoadedExecutable::execute(const DeviceView &device, const Buffer &inputBuffer) {
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

  // Argument lists: Our program takes 1 argument. We are executing on 1 device.
  PJRT_Buffer* actual_input_buffers_for_device0[] = {inputBuffer.c_buffer()}; // Use getter
  PJRT_Buffer* const* argument_lists_for_all_devices[] = {actual_input_buffers_for_device0}; // Array of lists of args
  exec_args.argument_lists = argument_lists_for_all_devices;
  exec_args.num_devices = 1; // We are launching on a single device instance here
  exec_args.num_args = 1;    // The @main function has one argument %arg0

  // Output setup
  PJRT_Buffer* raw_output_c_buffer = nullptr;
  PJRT_Buffer** output_list_for_device0[1];
  output_list_for_device0[0] = &raw_output_c_buffer;
  exec_args.output_lists = output_list_for_device0;

  // Device complete events: one event per device in the launch
  PJRT_Event* device_complete_event_handles[1];
  exec_args.device_complete_events = device_complete_event_handles;
  exec_args.execute_device = device.device_; // Execute on the specific device we chose

  PJRT_Error* exec_error = context_.pjrtApi_->PJRT_LoadedExecutable_Execute(&exec_args);
  if (exec_error != nullptr) {
    // It's possible that raw_output_c_buffer is populated even on error.
    // However, PJRT_Buffer_Destroy requires a valid PJRT_Buffer*.
    // If exec_error is not null, raw_output_c_buffer might not be valid or might be partially initialized.
    // The PJRT spec is unclear on whether PJRT_Buffer_Destroy should be called on outputs if Execute fails.
    // For now, we assume the C API handles cleanup of output buffers if Execute itself fails.
    throw context_.convertPjrtErrorToException(exec_error, "PJRT_LoadedExecutable_Execute", __FILE__, __LINE__);
  }

  // Get the output buffer dimensions using raw_output_c_buffer
  PJRT_Buffer_Dimensions_Args dim_args;
  dim_args.struct_size = PJRT_Buffer_Dimensions_Args_STRUCT_SIZE;
  dim_args.extension_start = nullptr;
  dim_args.buffer = raw_output_c_buffer; // Use the C buffer pointer populated by Execute
  dim_args.dims = nullptr;
  dim_args.num_dims = 0;

  // TODO: The PJRT_Buffer_Dimensions call itself might return an error in dim_args.error.
  // This needs to be checked.
  context_.pjrtApi_->PJRT_Buffer_Dimensions(&dim_args);
  if (dim_args.error != nullptr) {
    // If getting dimensions fails, we might have a valid PJRT_Buffer that needs destroying.
    // This is tricky. For now, throw, assuming the buffer might be in an inconsistent state.
    // A more robust solution might involve trying to destroy raw_output_c_buffer if it's non-null.
    throw context_.convertPjrtErrorToException(dim_args.error, "PJRT_Buffer_Dimensions (getting rank)", __FILE__, __LINE__);
  }

  size_t rank = dim_args.num_dims;
  std::vector<int64_t> actual_dimensions(rank);

  if (rank > 0) {
    dim_args.dims = actual_dimensions.data();
    dim_args.num_dims = rank; // Pass the capacity
    context_.pjrtApi_->PJRT_Buffer_Dimensions(&dim_args);
    if (dim_args.error != nullptr) {
      // Similar to above, error handling for partially created buffers is complex.
      throw context_.convertPjrtErrorToException(dim_args.error, "PJRT_Buffer_Dimensions (getting dims)", __FILE__, __LINE__);
    }
  }

  // Construct the final Buffer object
  pjrt::Buffer final_output_buffer(context_, raw_output_c_buffer, std::move(actual_dimensions));

  // Create CallbackUserData with the fully formed Buffer
  std::unique_ptr<detail::CallbackUserData<Buffer>> callbackUserData =
      std::make_unique<detail::CallbackUserData<Buffer>>(context_, std::move(final_output_buffer));

  return context_.getFutureForEvent(device_complete_event_handles[0], std::move(callbackUserData));
}


} // namespace pjrt