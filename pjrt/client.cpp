#include "client.hpp"
#include "common.hpp"
#include "context.hpp"
#include "event.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif

// Assume pjrt_c_api.h is in the same directory or an include path
#include "pjrt_c_api.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <cstring>
#include <stdexcept>
#include <vector>

namespace pjrt {

Client::Client(const Context &context) : context_(context) {
  PJRT_Client_Create_Args clientCreateArgs;

  // Initialize the struct. The PJRT_DEFINE_STRUCT_TRAITS macro in pjrt_c_api.h
  // defines e.g. PJRT_Client_Create_Args_STRUCT_SIZE which should be used.
  clientCreateArgs.struct_size = PJRT_Client_Create_Args_STRUCT_SIZE;
  clientCreateArgs.extension_start = nullptr;
  clientCreateArgs.create_options = nullptr; // No specific options for now
  clientCreateArgs.num_options = 0;
  clientCreateArgs.kv_get_callback = nullptr; // No distributed store for basic client
  clientCreateArgs.kv_get_user_arg = nullptr;
  clientCreateArgs.kv_put_callback = nullptr;
  clientCreateArgs.kv_put_user_arg = nullptr;
  clientCreateArgs.kv_try_get_callback = nullptr;
  clientCreateArgs.kv_try_get_user_arg = nullptr;
  // clientCreateArgs.client will be populated by the call

  PJRT_Error *clientCreateError = context_.pjrtApi_->PJRT_Client_Create(&clientCreateArgs);

  if (clientCreateError != nullptr) {
    throw std::runtime_error(freeErrorAndReturnString(context_, clientCreateError, "PJRT_Client_Create failed."));
  }

  client_ = clientCreateArgs.client; // Successfully created client is in the args struct
  if (!client_) {
    throw std::runtime_error("PJRT_Client_Create reported success, but the client pointer is null.");
  }
}

Client::~Client() {
  if (!client_) {
    return;
  }

  // Destroy the PJRT Client when done.
  PJRT_Client_Destroy_Args client_destroy_args;
  client_destroy_args.struct_size = PJRT_Client_Destroy_Args_STRUCT_SIZE;
  client_destroy_args.extension_start = nullptr;
  client_destroy_args.client = client_;
  PJRT_Error* clientDestroyError = context_.pjrtApi_->PJRT_Client_Destroy(&client_destroy_args);
  if (clientDestroyError) {
    throw std::runtime_error(freeErrorAndReturnString(context_, clientDestroyError, "PJRT_Client_Destroy failed."));
  }
}

std::string Client::platformName() const {
  PJRT_Client_PlatformName_Args platform_name_args;
  platform_name_args.struct_size = PJRT_Client_PlatformName_Args_STRUCT_SIZE;
  platform_name_args.extension_start = nullptr;
  platform_name_args.client = client_;

  PJRT_Error* platform_name_error = context_.pjrtApi_->PJRT_Client_PlatformName(&platform_name_args);
  if (platform_name_error != nullptr) {
    throw std::runtime_error(freeErrorAndReturnString(context_, platform_name_error, "PJRT_Client_PlatformName failed."));
  }
  return std::string(platform_name_args.platform_name, platform_name_args.platform_name_size);
}

LoadedExecutable Client::compileFromStableHloString(const std::string &stableHloProgram) const {
  // Use a std::vector<char> for PJRT_Program.code to be safe with the char* type
  std::vector<char> hlo_program_buffer(stableHloProgram.begin(), stableHloProgram.end());
  // PJRT_Program does not typically require a null terminator if code_size is accurate.
  // If your specific format or plugin did, you would add: hlo_program_buffer.push_back('\0');

  // --- Compile the Program ---
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
  compile_args.client = client_;
  compile_args.program = &program_desc;
  compile_args.compile_options = compile_options; // No specific compile options for now
  compile_args.compile_options_size = length;
  // compile_args.executable will be populated

  PJRT_Error* compile_error = context_.pjrtApi_->PJRT_Client_Compile(&compile_args);

  if (compile_error != nullptr) {
    throw std::runtime_error(freeErrorAndReturnString(context_, compile_error, "PJRT_Client_Compile failed."));
  }

  PJRT_LoadedExecutable *compiledExecutable = compile_args.executable;
  if (!compiledExecutable) {
    throw std::runtime_error("PJRT_Client_Compile reported success, but the executable pointer is null.");
  }

  return LoadedExecutable(context_, compiledExecutable);
}

DeviceView Client::getFirstDevice() const {
  // Get addressable devices.
  PJRT_Client_AddressableDevices_Args ad_args;
  ad_args.struct_size = PJRT_Client_AddressableDevices_Args_STRUCT_SIZE;
  ad_args.extension_start = nullptr;
  ad_args.client = client_;
  // ad_args.addressable_devices and ad_args.num_addressable_devices will be populated

  PJRT_Error* ad_error = context_.pjrtApi_->PJRT_Client_AddressableDevices(&ad_args);
  if (ad_error != nullptr) {
    throw std::runtime_error(freeErrorAndReturnString(context_, ad_error, "PJRT_Client_AddressableDevices failed."));
  }

  if (ad_args.num_addressable_devices == 0) {
    throw std::runtime_error("No addressable devices found.");
  }

  return DeviceView(context_, ad_args.addressable_devices[0]);
}

Buffer Client::createBufferFromData(float singleFloat, const DeviceView &device) const {
  // Create Input Buffer from Host Data
  PJRT_Client_BufferFromHostBuffer_Args bfhh_args;
  bfhh_args.struct_size = PJRT_Client_BufferFromHostBuffer_Args_STRUCT_SIZE;
  bfhh_args.extension_start = nullptr;
  bfhh_args.client = client_;
  bfhh_args.data = &singleFloat;
  bfhh_args.type = PJRT_Buffer_Type_F32;

  // For a scalar tensor<f32>, it's a rank-0 tensor.
  // int64_t input_dims[] = {}; // Empty for rank-0
  bfhh_args.dims = nullptr;
  bfhh_args.num_dims = 0; 

  bfhh_args.byte_strides = nullptr; // Dense layout
  bfhh_args.num_byte_strides = 0;
  bfhh_args.host_buffer_semantics = PJRT_HostBufferSemantics_kImmutableUntilTransferCompletes;
  bfhh_args.device = device.device_;
  bfhh_args.memory = nullptr; // Use device's default memory
  bfhh_args.device_layout = nullptr; // Use default layout

  // These fields will be populated by the API call
  bfhh_args.done_with_host_buffer = nullptr; 
  bfhh_args.buffer = nullptr;

  PJRT_Error* bfhh_error = context_.pjrtApi_->PJRT_Client_BufferFromHostBuffer(&bfhh_args);
  if (bfhh_error != nullptr) {
    throw std::runtime_error(freeErrorAndReturnString(context_, bfhh_error, "PJRT_Client_BufferFromHostBuffer failed."));
  }
  
  Buffer buffer(context_, bfhh_args.buffer);
  PJRT_Event* input_buffer_host_transfer_event = bfhh_args.done_with_host_buffer;
  if (input_buffer_host_transfer_event == nullptr) {
    throw std::runtime_error("Allocated buffer on device, but have no event to wait on for completion of transfer");
  }
  Event transferCompletedEvent(context_, input_buffer_host_transfer_event);
  // Wait for the host-to-device transfer to complete for the input buffer
  transferCompletedEvent.wait();

  return buffer;
}

} // namespace pjrt