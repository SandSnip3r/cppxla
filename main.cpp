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

#ifndef PJRT_PLUGIN_PATH
  #error "PJRT_PLUGIN_PATH is not defined. Please define it via CMake's target_compile_definitions."
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
std::string read_file_into_string(const std::string& path) {
  std::ifstream input_file(path);
  if (!input_file.is_open()) {
      throw std::runtime_error("Could not open file: " + path);
  }
  std::stringstream buffer;
  buffer << input_file.rdbuf();
  return buffer.str();
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <path_to_hlo_program_file>" << std::endl;
    std::cerr << "Example: " << argv[0] << " ../program.stablehlo  (if running from build dir)" << std::endl;
    return 1;
  }
  const char* hlo_program_file_path = argv[1];
  const char* plugin_path = PJRT_PLUGIN_PATH;
  void* plugin_handle = nullptr;
  const PJRT_Api* pjrt_api = nullptr;

  std::cout << "Attempting to load PJRT plugin: " << plugin_path << std::endl;

  // 1. Open the plugin
  // RTLD_LAZY: Resolve symbols only as the code that references them is executed.
  // RTLD_GLOBAL: Make symbols from this library available for symbol resolution of subsequently loaded libraries.
  plugin_handle = dlopen(plugin_path, RTLD_LAZY | RTLD_GLOBAL);
  if (!plugin_handle) {
    std::cerr << "Error loading PJRT plugin: " << dlerror() << std::endl;
    return 1;
  }
  std::cout << "Plugin loaded successfully." << std::endl;

  // Clear any existing error
  dlerror();

  // 2. Find the GetPjrtApi symbol
  GetPjrtApi_Func_Type GetPjrtApi_func = (GetPjrtApi_Func_Type)dlsym(plugin_handle, "GetPjrtApi");

  const char* dlsym_error = dlerror();
  if (dlsym_error) {
    std::cerr << "Error finding 'GetPjrtApi' symbol: " << dlsym_error << std::endl;
    dlclose(plugin_handle);
    return 1;
  }
  if (!GetPjrtApi_func) {
    std::cerr << "'GetPjrtApi' symbol found but the function pointer is null." << std::endl;
    dlclose(plugin_handle);
    return 1;
  }
  std::cout << "'GetPjrtApi' symbol found successfully." << std::endl;

  // 3. Call GetPjrtApi to get the API function table
  pjrt_api = GetPjrtApi_func();
  if (!pjrt_api) {
    std::cerr << "Call to GetPjrtApi_func() returned a null PJRT_Api pointer." << std::endl;
    dlclose(plugin_handle);
    return 1;
  }
  std::cout << "PJRT_Api pointer obtained successfully." << std::endl;

  // 4. Perform a small check: Print the API version
  // The pjrt_c_api.h should define PJRT_Api_Version struct and PJRT_Api struct containing it.
  if (pjrt_api->pjrt_api_version.major_version == 0 && pjrt_api->pjrt_api_version.minor_version == 0) {
    std::cout << "Warning: PJRT API version is 0.0. This might indicate an issue or an uninitialized struct if the plugin doesn't set it." << std::endl;
  } else {
    // TODO: Comapre to the version in our header.
    std::cout << "PJRT API Version: "
              << pjrt_api->pjrt_api_version.major_version << "."
              << pjrt_api->pjrt_api_version.minor_version << std::endl;
  }
  
  std::cout << "Successfully initialized and checked PJRT API." << std::endl;
  // --- At this point, you can start using other functions from pjrt_api ---
  // ========================================================================================

  // --- Step 1: Create a PJRT Client ---
  PJRT_Client* client = nullptr; // Will hold our created client
  std::cout << "Attempting to create PJRT_Client..." << std::endl;
  PJRT_Client_Create_Args client_create_args;
  // Initialize the struct. The PJRT_DEFINE_STRUCT_TRAITS macro in pjrt_c_api.h
  // defines e.g. PJRT_Client_Create_Args_STRUCT_SIZE which should be used.
  client_create_args.struct_size = PJRT_Client_Create_Args_STRUCT_SIZE;
  client_create_args.extension_start = nullptr;
  client_create_args.create_options = nullptr; // No specific options for now
  client_create_args.num_options = 0;
  client_create_args.kv_get_callback = nullptr; // No distributed store for basic client
  client_create_args.kv_get_user_arg = nullptr;
  client_create_args.kv_put_callback = nullptr;
  client_create_args.kv_put_user_arg = nullptr;
  client_create_args.kv_try_get_callback = nullptr;
  client_create_args.kv_try_get_user_arg = nullptr;
  // client_create_args.client will be populated by the call

  PJRT_Error* client_create_error = pjrt_api->PJRT_Client_Create(&client_create_args);

  if (client_create_error != nullptr) {
    handle_pjrt_error(client_create_error, pjrt_api, "PJRT_Client_Create failed.");
    dlclose(plugin_handle);
    return 1;
  }

  client = client_create_args.client; // Successfully created client is in the args struct
  if (!client) {
    std::cerr << "PJRT_Client_Create reported success, but the client pointer is null." << std::endl;
    dlclose(plugin_handle);
    return 1;
  }
  std::cout << "PJRT_Client created successfully! Client pointer: " << client << std::endl;

  // --- You can now use the client to get platform name, devices, etc. ---
  // Example: Get Platform Name
  PJRT_Client_PlatformName_Args platform_name_args;
  platform_name_args.struct_size = PJRT_Client_PlatformName_Args_STRUCT_SIZE;
  platform_name_args.extension_start = nullptr;
  platform_name_args.client = client;

  PJRT_Error* platform_name_error = pjrt_api->PJRT_Client_PlatformName(&platform_name_args);
  if (platform_name_error != nullptr) {
    handle_pjrt_error(platform_name_error, pjrt_api, "PJRT_Client_PlatformName failed.");
    // Continue to client cleanup
  } else {
    std::cout << "Platform Name: " << std::string(platform_name_args.platform_name, platform_name_args.platform_name_size) << std::endl;
  }

  // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-

  // --- Read HLO Program from File ---
  std::string hlo_program_str;
  try {
    hlo_program_str = read_file_into_string(hlo_program_file_path);
    std::cout << "Successfully read HLO program from: " << hlo_program_file_path << std::endl;
    // std::cout << "Program content:\n" << hlo_program_str << std::endl; // Optional: print content
  } catch (const std::exception& e) {
    std::cerr << "Error reading HLO file: " << e.what() << std::endl;
    if (client) { /* ... destroy client ... */
      PJRT_Client_Destroy_Args destroy_args; destroy_args.struct_size = PJRT_Client_Destroy_Args_STRUCT_SIZE;
      destroy_args.client = client; pjrt_api->PJRT_Client_Destroy(&destroy_args);
    }
    dlclose(plugin_handle); return 1;
  }

  // Use a std::vector<char> for PJRT_Program.code to be safe with the char* type
  std::vector<char> hlo_program_buffer(hlo_program_str.begin(), hlo_program_str.end());
  // PJRT_Program does not typically require a null terminator if code_size is accurate.
  // If your specific format or plugin did, you would add: hlo_program_buffer.push_back('\0');

  // --- Compile the Program ---
  std::cout << "Attempting to compile HLO program..." << std::endl;
  PJRT_Program program_desc;
  program_desc.struct_size = PJRT_Program_STRUCT_SIZE;
  program_desc.extension_start = nullptr;
  program_desc.code = hlo_program_buffer.data(); // data() from vector is char*
  program_desc.code_size = hlo_program_buffer.size(); // Use the actual size of the content in the vector
  
  const char* format_str = "mlir"; // Your program is in MHLO MLIR text format
  program_desc.format = format_str;
  program_desc.format_size = strlen(format_str);

  // I manually exported the string of the device config proto.
  const unsigned char compile_options_data[] = { 26,25,32,1,40,1,74,9,8,1,16,1,26,3,10,1,0,98,1,0,146,1,1,0,184,1,1 };
  const int length = sizeof(compile_options_data) / sizeof(compile_options_data[0]);
  const char* compile_options = reinterpret_cast<const char*>(compile_options_data);

  PJRT_Client_Compile_Args compile_args;
  compile_args.struct_size = PJRT_Client_Compile_Args_STRUCT_SIZE;
  compile_args.extension_start = nullptr;
  compile_args.client = client;
  compile_args.program = &program_desc;
  compile_args.compile_options = compile_options; // No specific compile options for now
  compile_args.compile_options_size = length;
  // compile_args.executable will be populated

  PJRT_Error* compile_error = pjrt_api->PJRT_Client_Compile(&compile_args);

  if (compile_error != nullptr) {
    handle_pjrt_error(compile_error, pjrt_api, "PJRT_Client_Compile failed.");
    // Cleanup client
    if (client) { /* ... destroy client ... */
        PJRT_Client_Destroy_Args destroy_args; destroy_args.struct_size = PJRT_Client_Destroy_Args_STRUCT_SIZE;
      destroy_args.client = client; pjrt_api->PJRT_Client_Destroy(&destroy_args);
    }
    dlclose(plugin_handle); return 1;
  }

  PJRT_LoadedExecutable *compiled_executable = compile_args.executable;
  if (!compiled_executable) {
    std::cerr << "PJRT_Client_Compile reported success, but the executable pointer is null." << std::endl;
    if (client) { /* ... destroy client ... */
      PJRT_Client_Destroy_Args destroy_args; destroy_args.struct_size = PJRT_Client_Destroy_Args_STRUCT_SIZE;
      destroy_args.client = client; pjrt_api->PJRT_Client_Destroy(&destroy_args);
    }
    dlclose(plugin_handle); return 1;
  }
  std::cout << "HLO program compiled successfully! Executable pointer: " << compiled_executable << std::endl;

  // ***************************************************************************************

  // (Assuming pjrt_api, client, and compiled_executable are valid at this point)
  // (Also assuming handle_pjrt_error function is defined as before)

  PJRT_Device* device = nullptr;
  PJRT_Buffer* input_buffer = nullptr;
  PJRT_Event* input_buffer_host_transfer_event = nullptr;
  PJRT_Buffer* output_buffer = nullptr;
  PJRT_Event* execution_complete_event = nullptr;
  PJRT_Event* output_buffer_host_transfer_event = nullptr;

  // --- Step 1: Get an Addressable Device ---
  std::cout << "Getting addressable devices..." << std::endl;
  PJRT_Client_AddressableDevices_Args ad_args;
  ad_args.struct_size = PJRT_Client_AddressableDevices_Args_STRUCT_SIZE;
  ad_args.extension_start = nullptr;
  ad_args.client = client;
  // ad_args.addressable_devices and ad_args.num_addressable_devices will be populated

  PJRT_Error* ad_error = pjrt_api->PJRT_Client_AddressableDevices(&ad_args);
  if (ad_error != nullptr) {
      handle_pjrt_error(ad_error, pjrt_api, "PJRT_Client_AddressableDevices failed.");
      // Perform cleanup of previously acquired resources (client, plugin handle) if exiting here
      return 1; // Or handle error appropriately
  }

  if (ad_args.num_addressable_devices == 0) {
      std::cerr << "No addressable devices found." << std::endl;
      // Perform cleanup
      return 1;
  }
  device = ad_args.addressable_devices[0]; // Use the first addressable device
  std::cout << "Using device: " << device << std::endl;
  // You can get more device info using pjrt_api->PJRT_Device_GetDescription if needed


  // --- Step 2: Create Input Buffer from Host Data ---
  std::cout << "Creating input buffer..." << std::endl;
  float host_input_data = 41.0f;
  PJRT_Client_BufferFromHostBuffer_Args bfhh_args;
  bfhh_args.struct_size = PJRT_Client_BufferFromHostBuffer_Args_STRUCT_SIZE;
  bfhh_args.extension_start = nullptr;
  bfhh_args.client = client;
  bfhh_args.data = &host_input_data;
  bfhh_args.type = PJRT_Buffer_Type_F32;

  // For a scalar tensor<f32>, it's a rank-0 tensor.
  // int64_t input_dims[] = {}; // Empty for rank-0
  bfhh_args.dims = nullptr;
  bfhh_args.num_dims = 0; 

  bfhh_args.byte_strides = nullptr; // Dense layout
  bfhh_args.num_byte_strides = 0;
  bfhh_args.host_buffer_semantics = PJRT_HostBufferSemantics_kImmutableUntilTransferCompletes;
  bfhh_args.device = device;
  bfhh_args.memory = nullptr; // Use device's default memory
  bfhh_args.device_layout = nullptr; // Use default layout

  // These fields will be populated by the API call
  bfhh_args.done_with_host_buffer = nullptr; 
  bfhh_args.buffer = nullptr;

  PJRT_Error* bfhh_error = pjrt_api->PJRT_Client_BufferFromHostBuffer(&bfhh_args);
  if (bfhh_error != nullptr) {
      handle_pjrt_error(bfhh_error, pjrt_api, "PJRT_Client_BufferFromHostBuffer failed.");
      // Perform cleanup
      return 1;
  }
  input_buffer = bfhh_args.buffer;
  input_buffer_host_transfer_event = bfhh_args.done_with_host_buffer;
  std::cout << "Input buffer created. Host transfer event: " << input_buffer_host_transfer_event << std::endl;

  // Wait for the host-to-device transfer to complete for the input buffer
  if (input_buffer_host_transfer_event) {
      std::cout << "Awaiting input buffer host transfer event..." << std::endl;
      PJRT_Event_Await_Args await_args;
      await_args.struct_size = PJRT_Event_Await_Args_STRUCT_SIZE;
      await_args.extension_start = nullptr;
      await_args.event = input_buffer_host_transfer_event;
      PJRT_Error* await_error = pjrt_api->PJRT_Event_Await(&await_args);
      if (await_error) {
          handle_pjrt_error(await_error, pjrt_api, "PJRT_Event_Await for input buffer host transfer failed.");
          // Perform cleanup including input_buffer and the event itself
          if (input_buffer) { /* destroy input_buffer */ }
          pjrt_api->PJRT_Event_Destroy(&(PJRT_Event_Destroy_Args){PJRT_Event_Destroy_Args_STRUCT_SIZE, nullptr, input_buffer_host_transfer_event});
          return 1;
      }
      std::cout << "Input buffer host transfer complete." << std::endl;

      PJRT_Event_Destroy_Args event_destroy_args;
      event_destroy_args.struct_size = PJRT_Event_Destroy_Args_STRUCT_SIZE;
      event_destroy_args.extension_start = nullptr;
      event_destroy_args.event = input_buffer_host_transfer_event;
      pjrt_api->PJRT_Event_Destroy(&event_destroy_args); // Error on event destroy is usually just logged
      input_buffer_host_transfer_event = nullptr;
  }


  // --- Step 3: Prepare and Execute the Compiled Program ---
  std::cout << "Executing compiled program..." << std::endl;
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
  exec_args.executable = compiled_executable;
  exec_args.options = &exec_options;

  // Argument lists: Our program takes 1 argument. We are executing on 1 device.
  PJRT_Buffer* actual_input_buffers_for_device0[] = {input_buffer};
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
  exec_args.execute_device = device; // Execute on the specific device we chose

  PJRT_Error* exec_error = pjrt_api->PJRT_LoadedExecutable_Execute(&exec_args);
  if (exec_error != nullptr) {
      handle_pjrt_error(exec_error, pjrt_api, "PJRT_LoadedExecutable_Execute failed.");
      // Perform cleanup of input_buffer etc.
      if (input_buffer) { /* destroy input_buffer */ }
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
      PJRT_Error* await_exec_error = pjrt_api->PJRT_Event_Await(&await_exec_args);

      // Check the status of the execution from the event itself
      PJRT_Event_Error_Args event_error_args;
      event_error_args.struct_size = PJRT_Event_Error_Args_STRUCT_SIZE;
      event_error_args.extension_start = nullptr;
      event_error_args.event = execution_complete_event;
      PJRT_Error* exec_status_error = pjrt_api->PJRT_Event_Error(&event_error_args); // This gets the error *from* the event

      if (await_exec_error != nullptr) { // Error in the Await call itself
          handle_pjrt_error(await_exec_error, pjrt_api, "PJRT_Event_Await for execution completion failed.");
          if (exec_status_error) pjrt_api->PJRT_Error_Destroy(&(PJRT_Error_Destroy_Args){PJRT_Event_Destroy_Args_STRUCT_SIZE, nullptr, exec_status_error});
          // Perform cleanup
          if (input_buffer) { /* ... */ }
          if (output_buffer) { /* ... */ } // Output buffer might exist even if event had error
          pjrt_api->PJRT_Event_Destroy(&(PJRT_Event_Destroy_Args){PJRT_Event_Destroy_Args_STRUCT_SIZE, nullptr, execution_complete_event});
          return 1;
      }
      if (exec_status_error != nullptr) { // Execution itself resulted in an error
          handle_pjrt_error(exec_status_error, pjrt_api, "Execution completed with an error state.");
          // Perform cleanup
          if (input_buffer) { /* ... */ }
          if (output_buffer) { /* ... */ }
          pjrt_api->PJRT_Event_Destroy(&(PJRT_Event_Destroy_Args){PJRT_Event_Destroy_Args_STRUCT_SIZE, nullptr, execution_complete_event});
          return 1;
      }
      std::cout << "Execution completed successfully." << std::endl;
      
      // Destroy the execution complete event
      PJRT_Event_Destroy_Args event_destroy_args;
      event_destroy_args.struct_size = PJRT_Event_Destroy_Args_STRUCT_SIZE;
      event_destroy_args.extension_start = nullptr;
      event_destroy_args.event = execution_complete_event;
      pjrt_api->PJRT_Event_Destroy(&event_destroy_args);
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

      PJRT_Error* bthh_error = pjrt_api->PJRT_Buffer_ToHostBuffer(&bthh_args);
      if (bthh_error != nullptr) {
          handle_pjrt_error(bthh_error, pjrt_api, "PJRT_Buffer_ToHostBuffer failed.");
          // Perform cleanup
          if (input_buffer) { /* ... */ }
          pjrt_api->PJRT_Buffer_Destroy(&(PJRT_Buffer_Destroy_Args){PJRT_Buffer_Destroy_Args_STRUCT_SIZE, nullptr, output_buffer});
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
          PJRT_Error* await_output_error = pjrt_api->PJRT_Event_Await(&await_output_args);
          if (await_output_error) {
              handle_pjrt_error(await_output_error, pjrt_api, "PJRT_Event_Await for output buffer host transfer failed.");
              // Perform cleanup
          } else {
              std::cout << "Output buffer host transfer complete." << std::endl;
              std::cout << "Output value: " << host_output_data << std::endl; // Expected: 42.0
          }
          
          PJRT_Event_Destroy_Args event_destroy_args;
          event_destroy_args.struct_size = PJRT_Event_Destroy_Args_STRUCT_SIZE;
          event_destroy_args.extension_start = nullptr;
          event_destroy_args.event = output_buffer_host_transfer_event;
          pjrt_api->PJRT_Event_Destroy(&event_destroy_args);
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
      PJRT_Error* error = pjrt_api->PJRT_Buffer_Destroy(&destroy_args);
      if(error) handle_pjrt_error(error, pjrt_api, "Failed to destroy output buffer");
      else std::cout << "Output buffer destroyed." << std::endl;
      output_buffer = nullptr;
  }
  if (input_buffer) {
      std::cout << "Destroying input buffer..." << std::endl;
      PJRT_Buffer_Destroy_Args destroy_args;
      destroy_args.struct_size = PJRT_Buffer_Destroy_Args_STRUCT_SIZE;
      destroy_args.extension_start = nullptr;
      destroy_args.buffer = input_buffer;
      PJRT_Error* error = pjrt_api->PJRT_Buffer_Destroy(&destroy_args);
      if(error) handle_pjrt_error(error, pjrt_api, "Failed to destroy input buffer");
      else std::cout << "Input buffer destroyed." << std::endl;
      input_buffer = nullptr;
  }

  // Any remaining events should have been destroyed.
  // If any error occurred mid-way, ensure all previously created resources are cleaned up.
  // The full main function would need more robust try/catch or RAII for this.
  
  // ***************************************************************************************

  // --- Cleanup ---
  if (compiled_executable) {
    std::cout << "Destroying compiled executable..." << std::endl;
    PJRT_LoadedExecutable_Destroy_Args exec_destroy_args;
    exec_destroy_args.struct_size = PJRT_LoadedExecutable_Destroy_Args_STRUCT_SIZE;
    exec_destroy_args.extension_start = nullptr;
    exec_destroy_args.executable = compiled_executable;
    PJRT_Error* exec_destroy_error = pjrt_api->PJRT_LoadedExecutable_Destroy(&exec_destroy_args);
    if (exec_destroy_error) {
      handle_pjrt_error(exec_destroy_error, pjrt_api, "PJRT_LoadedExecutable_Destroy failed.");
    } else {
      std::cout << "Compiled executable destroyed." << std::endl;
    }
  }

  // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-

  // Destroy the PJRT Client when done.
  std::cout << "Attempting to destroy PJRT_Client..." << std::endl;
  if (client) {
    std::cout << "Destroying PJRT_Client..." << std::endl;
    PJRT_Client_Destroy_Args client_destroy_args;
    client_destroy_args.struct_size = PJRT_Client_Destroy_Args_STRUCT_SIZE;
    client_destroy_args.extension_start = nullptr;
    client_destroy_args.client = client;
    PJRT_Error* client_destroy_error = pjrt_api->PJRT_Client_Destroy(&client_destroy_args);
    if (client_destroy_error) {
      handle_pjrt_error(client_destroy_error, pjrt_api, "PJRT_Client_Destroy failed.");
    } else {
      std::cout << "PJRT_Client destroyed." << std::endl;
    }
  }

  // ========================================================================================

  // 5. Close the plugin
  if (dlclose(plugin_handle) != 0) {
    std::cerr << "Error closing PJRT plugin: " << dlerror() << std::endl;
    // Continue, as we might have already succeeded with API usage
  } else {
    std::cout << "Plugin closed successfully." << std::endl;
  }

  return 0;
}