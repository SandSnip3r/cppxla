#include "common.hpp"
#include "context.hpp"
#include "device.hpp"
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

namespace pjrt {

LoadedExecutable::LoadedExecutable(const Context &context, PJRT_LoadedExecutable *executable) : context_(context), executable_(executable) {

}

LoadedExecutable::~LoadedExecutable() {
  if (!executable_) {
    return;
  }
  PJRT_LoadedExecutable_Destroy_Args exec_destroy_args;
  exec_destroy_args.struct_size = PJRT_LoadedExecutable_Destroy_Args_STRUCT_SIZE;
  exec_destroy_args.extension_start = nullptr;
  exec_destroy_args.executable = executable_;
  PJRT_Error* exec_destroy_error = context_.pjrtApi_->PJRT_LoadedExecutable_Destroy(&exec_destroy_args);
  if (exec_destroy_error) {
    throw std::runtime_error(freeErrorAndReturnString(context_, exec_destroy_error, "PJRT_LoadedExecutable_Destroy failed."));
  }
}

Buffer LoadedExecutable::execute(const Device &device, const Buffer &inputBuffer) {
  return {context_, nullptr};
  // PJRT_Buffer* output_buffer = nullptr;
  // // PJRT_Event* execution_complete_event = nullptr;
  // PJRT_Event* output_buffer_host_transfer_event = nullptr;


  // // --- Step 3: Prepare and Execute the Compiled Program ---
  // PJRT_ExecuteOptions exec_options;
  // exec_options.struct_size = PJRT_ExecuteOptions_STRUCT_SIZE;
  // exec_options.extension_start = nullptr;
  // exec_options.launch_id = 0; // Default launch ID
  // exec_options.num_send_ops = 0;
  // exec_options.send_callbacks = nullptr;
  // exec_options.num_recv_ops = 0;
  // exec_options.recv_callbacks = nullptr;
  // exec_options.non_donatable_input_indices = nullptr;
  // exec_options.num_non_donatable_input_indices = 0;
  // exec_options.context = nullptr;


  // PJRT_LoadedExecutable_Execute_Args exec_args;
  // exec_args.struct_size = PJRT_LoadedExecutable_Execute_Args_STRUCT_SIZE;
  // exec_args.extension_start = nullptr;
  // exec_args.executable = executable_;
  // exec_args.options = &exec_options;

  // // Argument lists: Our program takes 1 argument. We are executing on 1 device.
  // PJRT_Buffer* actual_input_buffers_for_device0[] = {inputBuffer.buffer_};
  // PJRT_Buffer* const* argument_lists_for_all_devices[] = {actual_input_buffers_for_device0}; // Array of lists of args
  // exec_args.argument_lists = argument_lists_for_all_devices;
  // exec_args.num_devices = 1; // We are launching on a single device instance here
  // exec_args.num_args = 1;    // The @main function has one argument %arg0

  // // Output lists: Our program has 1 output.
  // // The API will populate the PJRT_Buffer* in this array.
  // PJRT_Buffer* output_buffer_handles_for_device0[1]; // For 1 output from the function on device 0
  // PJRT_Buffer** output_lists_for_all_devices[1];    // Array of lists of outputs
  // output_lists_for_all_devices[0] = output_buffer_handles_for_device0;
  // exec_args.output_lists = output_lists_for_all_devices;

  // // Device complete events: one event per device in the launch
  // PJRT_Event* device_complete_event_handles[1];
  // exec_args.device_complete_events = device_complete_event_handles;
  // exec_args.execute_device = device.device_; // Execute on the specific device we chose

  // PJRT_Error* exec_error = context_.pjrtApi_->PJRT_LoadedExecutable_Execute(&exec_args);
  // if (exec_error != nullptr) {
  //   throw std::runtime_error(freeErrorAndReturnString(context_, exec_error, "PJRT_LoadedExecutable_Execute failed."));
  // }

  // // The actual PJRT_Event* and PJRT_Buffer* for the first (and only) device and first output
  // // execution_complete_event = device_complete_event_handles[0];
  // // output_buffer = output_buffer_handles_for_device0[0];
  // Event executionCompleteEvent(context_, device_complete_event_handles[0]);
  // Buffer outputBuffer(context_, output_buffer_handles_for_device0[0]);

  // // Wait for execution to complete
  // if (execution_complete_event) {
  //   std::cout << "Awaiting execution completion event..." << std::endl;
  //   PJRT_Event_Await_Args await_exec_args;
  //   await_exec_args.struct_size = PJRT_Event_Await_Args_STRUCT_SIZE;
  //   await_exec_args.extension_start = nullptr;
  //   await_exec_args.event = execution_complete_event;
  //   PJRT_Error* await_exec_error = context.pjrtApi_->PJRT_Event_Await(&await_exec_args);

  //   // Check the status of the execution from the event itself
  //   PJRT_Event_Error_Args event_error_args;
  //   event_error_args.struct_size = PJRT_Event_Error_Args_STRUCT_SIZE;
  //   event_error_args.extension_start = nullptr;
  //   event_error_args.event = execution_complete_event;
  //   PJRT_Error* exec_status_error = context.pjrtApi_->PJRT_Event_Error(&event_error_args); // This gets the error *from* the event

  //   if (await_exec_error != nullptr) { // Error in the Await call itself
  //     handle_pjrt_error(await_exec_error, context.pjrtApi_, "PJRT_Event_Await for execution completion failed.");
  //     if (exec_status_error) {
  //       PJRT_Error_Destroy_Args tmp{PJRT_Event_Destroy_Args_STRUCT_SIZE, nullptr, exec_status_error};
  //       context.pjrtApi_->PJRT_Error_Destroy(&tmp);
  //     }
  //     // Perform cleanup
  //     if (output_buffer) {
  //       // Output buffer might exist even if event had error
  //       /* ... */
  //     }
  //     PJRT_Event_Destroy_Args tmp{PJRT_Event_Destroy_Args_STRUCT_SIZE, nullptr, execution_complete_event};
  //     context.pjrtApi_->PJRT_Event_Destroy(&tmp);
  //     return 1;
  //   }
  //   if (exec_status_error != nullptr) { // Execution itself resulted in an error
  //     handle_pjrt_error(exec_status_error, context.pjrtApi_, "Execution completed with an error state.");
  //     // Perform cleanup
  //     if (output_buffer) {
  //       /* ... */
  //     }
  //     PJRT_Event_Destroy_Args tmp{PJRT_Event_Destroy_Args_STRUCT_SIZE, nullptr, execution_complete_event};
  //     context.pjrtApi_->PJRT_Event_Destroy(&tmp);
  //     return 1;
  //   }
  //   std::cout << "Execution completed successfully." << std::endl;
    
  //   // Destroy the execution complete event
  //   PJRT_Event_Destroy_Args event_destroy_args;
  //   event_destroy_args.struct_size = PJRT_Event_Destroy_Args_STRUCT_SIZE;
  //   event_destroy_args.extension_start = nullptr;
  //   event_destroy_args.event = execution_complete_event;
  //   context.pjrtApi_->PJRT_Event_Destroy(&event_destroy_args);
  //   execution_complete_event = nullptr;
  // }

}

  
} // namespace pjrt