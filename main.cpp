#include "pjrt/client.hpp"
#include "pjrt/context.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <dlfcn.h> // For dlopen, dlsym, dlclose, dlerror

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif

// Assume pjrt_c_api.h is in the same directory or an include path
#include "pjrt_c_api.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

// Define the function pointer type for GetPjrtApi
typedef const PJRT_Api* (*GetPjrtApi_Func_Type)();

// Helper function to print PJRT_Error (if any) and destroy it
void handle_pjrt_error(PJRT_Error* error, const PJRT_Api* pjrt_api, const std::string& context_message) {
  if (error) {
    PJRT_Error_Message_Args error_message_args;
    error_message_args.struct_size = PJRT_Error_Message_Args_STRUCT_SIZE;
    error_message_args.extension_start = nullptr;
    error_message_args.error = error;
    pjrt_api->PJRT_Error_Message(&error_message_args);

    std::cerr << context_message << " Error: "
              << std::string(error_message_args.message, error_message_args.message_size) << std::endl;

    PJRT_Error_Destroy_Args destroy_error_args;
    destroy_error_args.struct_size = PJRT_Error_Destroy_Args_STRUCT_SIZE;
    destroy_error_args.extension_start = nullptr;
    destroy_error_args.error = error;
    pjrt_api->PJRT_Error_Destroy(&destroy_error_args);
  }
}

// Function to read a file into a string
std::string readFileIntoStream(const std::string& path) {
  std::ifstream input_file(path);
  if (!input_file.is_open()) {
      throw std::runtime_error("Could not open file: " + path);
  }
  std::stringstream buffer;
  buffer << input_file.rdbuf();
  return buffer.str();
}

void performVersionCheck(pjrt::Context &context) {
  // 4. Perform a small check: Print the API version
  // The pjrt_c_api.h should define PJRT_Api_Version struct and PJRT_Api struct containing it.
  if (context.pjrtApi_->pjrt_api_version.major_version == 0 && context.pjrtApi_->pjrt_api_version.minor_version == 0) {
    std::cout << "Warning: PJRT API version is 0.0. This might indicate an issue or an uninitialized struct if the plugin doesn't set it." << std::endl;
  } else {
    // TODO: Comapre to the version in our header.
    std::cout << "PJRT API Version: "
              << context.pjrtApi_->pjrt_api_version.major_version << "."
              << context.pjrtApi_->pjrt_api_version.minor_version << std::endl;
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <path_to_hlo_program_file>" << std::endl;
    std::cerr << "Example: " << argv[0] << " ../program.stablehlo  (if running from build dir)" << std::endl;
    return 1;
  }

  const char* hloProgramFilePath = argv[1];
  
  pjrt::Context context;
  performVersionCheck(context);
  std::cout << "Successfully initialized and checked PJRT API." << std::endl;

  pjrt::Client client(context);
  std::cout << "PJRT_Client created successfully!" << std::endl;
  std::cout << "Platform Name: " << client.platformName() << std::endl;

  // --- Read HLO Program from File ---
  std::string stableHloStr;
  try {
    stableHloStr = readFileIntoStream(hloProgramFilePath);
    std::cout << "Successfully read HLO program from: " << hloProgramFilePath << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Error reading HLO file: " << e.what() << std::endl;
    return 1;
  }

  pjrt::LoadedExecutable executable = client.compileFromStableHloString(stableHloStr);
  pjrt::Device device = client.getFirstDevice();
  std::cout << "Created device" << std::endl;

  std::cout << "Creating input buffer..." << std::endl;
  float host_input_data = 41.0f;
  pjrt::Buffer input_buffer = client.createBufferFromData(host_input_data, device);
  std::cout << "Input buffer host transfer complete." << std::endl;

  std::cout << "Executing compiled program..." << std::endl;
  PJRT_Buffer* output_buffer = nullptr;
  PJRT_Event* execution_complete_event = nullptr;
  PJRT_Event* output_buffer_host_transfer_event = nullptr;


  // --- Step 3: Prepare and Execute the Compiled Program ---
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
  exec_args.executable = executable.executable_;
  exec_args.options = &exec_options;

  // Argument lists: Our program takes 1 argument. We are executing on 1 device.
  PJRT_Buffer* actual_input_buffers_for_device0[] = {input_buffer.buffer_};
  PJRT_Buffer* const* argument_lists_for_all_devices[] = {actual_input_buffers_for_device0}; // Array of lists of args
  exec_args.argument_lists = argument_lists_for_all_devices;
  exec_args.num_devices = 1; // We are launching on a single device instance here
  exec_args.num_args = 1;    // The @main function has one argument %arg0

  // Output lists: Our program has 1 output.
  // The API will populate the PJRT_Buffer* in this array.
  PJRT_Buffer* output_buffer_handles_for_device0[1]; // For 1 output from the function on device 0
  PJRT_Buffer** output_lists_for_all_devices[1];    // Array of lists of outputs
  output_lists_for_all_devices[0] = output_buffer_handles_for_device0;
  exec_args.output_lists = output_lists_for_all_devices;

  // Device complete events: one event per device in the launch
  PJRT_Event* device_complete_event_handles[1];
  exec_args.device_complete_events = device_complete_event_handles;
  exec_args.execute_device = device.device_; // Execute on the specific device we chose

  PJRT_Error* exec_error = context.pjrtApi_->PJRT_LoadedExecutable_Execute(&exec_args);
  if (exec_error != nullptr) {
      handle_pjrt_error(exec_error, context.pjrtApi_, "PJRT_LoadedExecutable_Execute failed.");
      // Perform cleanup of input_buffer etc.
      return 1;
  }
  std::cout << "Execution submitted." << std::endl;

  // The actual PJRT_Event* and PJRT_Buffer* for the first (and only) device and first output
  execution_complete_event = device_complete_event_handles[0];
  output_buffer = output_buffer_handles_for_device0[0];

  // Wait for execution to complete
  if (execution_complete_event) {
    std::cout << "Awaiting execution completion event..." << std::endl;
    PJRT_Event_Await_Args await_exec_args;
    await_exec_args.struct_size = PJRT_Event_Await_Args_STRUCT_SIZE;
    await_exec_args.extension_start = nullptr;
    await_exec_args.event = execution_complete_event;
    PJRT_Error* await_exec_error = context.pjrtApi_->PJRT_Event_Await(&await_exec_args);

    // Check the status of the execution from the event itself
    PJRT_Event_Error_Args event_error_args;
    event_error_args.struct_size = PJRT_Event_Error_Args_STRUCT_SIZE;
    event_error_args.extension_start = nullptr;
    event_error_args.event = execution_complete_event;
    PJRT_Error* exec_status_error = context.pjrtApi_->PJRT_Event_Error(&event_error_args); // This gets the error *from* the event

    if (await_exec_error != nullptr) { // Error in the Await call itself
      handle_pjrt_error(await_exec_error, context.pjrtApi_, "PJRT_Event_Await for execution completion failed.");
      if (exec_status_error) {
        PJRT_Error_Destroy_Args tmp{PJRT_Event_Destroy_Args_STRUCT_SIZE, nullptr, exec_status_error};
        context.pjrtApi_->PJRT_Error_Destroy(&tmp);
      }
      // Perform cleanup
      if (output_buffer) {
        // Output buffer might exist even if event had error
        /* ... */
      }
      PJRT_Event_Destroy_Args tmp{PJRT_Event_Destroy_Args_STRUCT_SIZE, nullptr, execution_complete_event};
      context.pjrtApi_->PJRT_Event_Destroy(&tmp);
      return 1;
    }
    if (exec_status_error != nullptr) { // Execution itself resulted in an error
      handle_pjrt_error(exec_status_error, context.pjrtApi_, "Execution completed with an error state.");
      // Perform cleanup
      if (output_buffer) {
        /* ... */
      }
      PJRT_Event_Destroy_Args tmp{PJRT_Event_Destroy_Args_STRUCT_SIZE, nullptr, execution_complete_event};
      context.pjrtApi_->PJRT_Event_Destroy(&tmp);
      return 1;
    }
    std::cout << "Execution completed successfully." << std::endl;
    
    // Destroy the execution complete event
    PJRT_Event_Destroy_Args event_destroy_args;
    event_destroy_args.struct_size = PJRT_Event_Destroy_Args_STRUCT_SIZE;
    event_destroy_args.extension_start = nullptr;
    event_destroy_args.event = execution_complete_event;
    context.pjrtApi_->PJRT_Event_Destroy(&event_destroy_args);
    execution_complete_event = nullptr;
  }


  // --- Step 4: Retrieve Output Buffer Data to Host ---
  if (output_buffer) {
    std::cout << "Retrieving output buffer data to host..." << std::endl;
    float host_output_data = 0.0f; // To store the scalar f32 result

    PJRT_Buffer_ToHostBuffer_Args bthh_args;
    bthh_args.struct_size = PJRT_Buffer_ToHostBuffer_Args_STRUCT_SIZE;
    bthh_args.extension_start = nullptr;
    bthh_args.src = output_buffer;
    bthh_args.dst = &host_output_data;
    bthh_args.dst_size = sizeof(float); // Size of our host buffer
    bthh_args.host_layout = nullptr; // Use default/source layout
    // bthh_args.event will be populated

    PJRT_Error* bthh_error = context.pjrtApi_->PJRT_Buffer_ToHostBuffer(&bthh_args);
    if (bthh_error != nullptr) {
      handle_pjrt_error(bthh_error, context.pjrtApi_, "PJRT_Buffer_ToHostBuffer failed.");
      // Perform cleanup
      PJRT_Buffer_Destroy_Args tmp{PJRT_Buffer_Destroy_Args_STRUCT_SIZE, nullptr, output_buffer};
      context.pjrtApi_->PJRT_Buffer_Destroy(&tmp);
      return 1;
    }
    output_buffer_host_transfer_event = bthh_args.event;
    std::cout << "Output buffer to host transfer submitted. Event: " << output_buffer_host_transfer_event << std::endl;

    // Wait for the device-to-host transfer to complete
    if (output_buffer_host_transfer_event) {
      std::cout << "Awaiting output buffer host transfer event..." << std::endl;
      PJRT_Event_Await_Args await_output_args;
      await_output_args.struct_size = PJRT_Event_Await_Args_STRUCT_SIZE;
      await_output_args.extension_start = nullptr;
      await_output_args.event = output_buffer_host_transfer_event;
      PJRT_Error* await_output_error = context.pjrtApi_->PJRT_Event_Await(&await_output_args);
      if (await_output_error) {
        handle_pjrt_error(await_output_error, context.pjrtApi_, "PJRT_Event_Await for output buffer host transfer failed.");
        // Perform cleanup
      } else {
        std::cout << "Output buffer host transfer complete." << std::endl;
        std::cout << "Output value: " << host_output_data << std::endl; // Expected: 42.0
      }
      
      PJRT_Event_Destroy_Args event_destroy_args;
      event_destroy_args.struct_size = PJRT_Event_Destroy_Args_STRUCT_SIZE;
      event_destroy_args.extension_start = nullptr;
      event_destroy_args.event = output_buffer_host_transfer_event;
      context.pjrtApi_->PJRT_Event_Destroy(&event_destroy_args);
      output_buffer_host_transfer_event = nullptr;
    }
  }


  // --- Step 5: Cleanup Buffers (input and output) ---
  // (Remember compiled_executable, client, and plugin_handle are cleaned up by the surrounding main function code)
  if (output_buffer) {
    std::cout << "Destroying output buffer..." << std::endl;
    PJRT_Buffer_Destroy_Args destroy_args;
    destroy_args.struct_size = PJRT_Buffer_Destroy_Args_STRUCT_SIZE;
    destroy_args.extension_start = nullptr;
    destroy_args.buffer = output_buffer;
    PJRT_Error* error = context.pjrtApi_->PJRT_Buffer_Destroy(&destroy_args);
    if (error) {
      handle_pjrt_error(error, context.pjrtApi_, "Failed to destroy output buffer");
    } else {
      std::cout << "Output buffer destroyed." << std::endl;
    }
    output_buffer = nullptr;
  }

  // Any remaining events should have been destroyed.
  // If any error occurred mid-way, ensure all previously created resources are cleaned up.
  // The full main function would need more robust try/catch or RAII for this.

  return 0;
}