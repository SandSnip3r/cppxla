#include "client.hpp"
#include "context.hpp"
#include "event.hpp"

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
    throw context_.convertPjrtErrorToException(clientCreateError, "PJRT_Client_Create", __FILE__, __LINE__);
  }

  client_ = clientCreateArgs.client; // Successfully created client is in the args struct
  if (!client_) {
    throw pjrt::Exception("PJRT_Client_Create reported success, but the client pointer is null.");
  }
}

Client::~Client() {
  if (client_ == nullptr) {
    return;
  }

  // Destroy the PJRT Client when done.
  PJRT_Client_Destroy_Args client_destroy_args;
  client_destroy_args.struct_size = PJRT_Client_Destroy_Args_STRUCT_SIZE;
  client_destroy_args.extension_start = nullptr;
  client_destroy_args.client = client_;
  PJRT_Error* clientDestroyError = context_.pjrtApi_->PJRT_Client_Destroy(&client_destroy_args);
  if (clientDestroyError != nullptr) {
    const pjrt::Exception ex = context_.convertPjrtErrorToException(clientDestroyError, "PJRT_Client_Destroy", __FILE__, __LINE__);
    std::cerr << "pjrt::Client destructor failed to destroy PJRT_Client: \"" << ex.what() << "\"" << std::endl;
  }
}

void Client::destroy() {
  if (client_ == nullptr) {
    return;
  }

  // Destroy the PJRT Client when done.
  PJRT_Client_Destroy_Args client_destroy_args;
  client_destroy_args.struct_size = PJRT_Client_Destroy_Args_STRUCT_SIZE;
  client_destroy_args.extension_start = nullptr;
  client_destroy_args.client = client_;
  PJRT_Error* clientDestroyError = context_.pjrtApi_->PJRT_Client_Destroy(&client_destroy_args);
  if (clientDestroyError != nullptr) {
    throw context_.convertPjrtErrorToException(clientDestroyError, "PJRT_Client_Destroy", __FILE__, __LINE__);
  }
}

std::string Client::platformName() const {
  PJRT_Client_PlatformName_Args platform_name_args;
  platform_name_args.struct_size = PJRT_Client_PlatformName_Args_STRUCT_SIZE;
  platform_name_args.extension_start = nullptr;
  platform_name_args.client = client_;

  PJRT_Error* platform_name_error = context_.pjrtApi_->PJRT_Client_PlatformName(&platform_name_args);
  if (platform_name_error != nullptr) {
    throw context_.convertPjrtErrorToException(platform_name_error, "PJRT_Client_PlatformName", __FILE__, __LINE__);
  }
  return std::string(platform_name_args.platform_name, platform_name_args.platform_name_size);
}

LoadedExecutable Client::compileFromStableHloString(const std::string &stableHloProgram) const {
  // Use a std::vector<char> for PJRT_Program.code to be safe with the char* type
  std::vector<char> hlo_program_buffer(stableHloProgram.begin(), stableHloProgram.end());
  // TODO(PJRT): It should be made clear that a null terminator is required on the program string. 
  hlo_program_buffer.push_back('\0');

  // --- Compile the Program ---
  PJRT_Program program_desc;
  program_desc.struct_size = PJRT_Program_STRUCT_SIZE;
  program_desc.extension_start = nullptr;
  program_desc.code = hlo_program_buffer.data();
  program_desc.code_size = hlo_program_buffer.size() - 1; // Use the actual size of the content in the vector (excluding the null terminator)

  const char* format_str = "mlir"; // Your program is in MHLO MLIR text format
  program_desc.format = format_str;
  program_desc.format_size = strlen(format_str);

  // I manually exported the string of the device config proto.
  const unsigned char compileOptionsData[] = { 26, 128, 7, 8, 255, 255, 255, 255, 255, 255, 255, 255, 255, 1, 26, 209, 6, 248, 1, 3, 152, 2, 1, 224, 3, 1, 234, 3, 93, 47, 117, 115, 114, 47, 108, 111, 99, 97, 108, 47, 103, 111, 111, 103, 108, 101, 47, 104, 111, 109, 101, 47, 118, 105, 99, 116, 111, 114, 115, 116, 111, 110, 101, 47, 99, 112, 112, 120, 108, 97, 47, 46, 118, 101, 110, 118, 47, 108, 105, 98, 47, 112, 121, 116, 104, 111, 110, 51, 46, 49, 51, 47, 115, 105, 116, 101, 45, 112, 97, 99, 107, 97, 103, 101, 115, 47, 110, 118, 105, 100, 105, 97, 47, 99, 117, 100, 97, 95, 110, 118, 99, 99, 176, 4, 1, 184, 4, 1, 192, 4, 1, 200, 4, 0, 136, 6, 0, 152, 6, 0, 160, 6, 0, 176, 6, 1, 160, 7, 0, 192, 7, 1, 200, 7, 1, 208, 7, 1, 216, 7, 4, 240, 7, 1, 136, 8, 1, 152, 8, 0, 160, 8, 255, 255, 255, 255, 255, 255, 255, 255, 255, 1, 200, 8, 0, 208, 8, 0, 224, 8, 0, 240, 8, 255, 255, 255, 255, 255, 255, 255, 255, 255, 1, 128, 9, 0, 168, 9, 0, 224, 9, 1, 232, 9, 135, 128, 128, 15, 152, 10, 255, 255, 255, 255, 255, 255, 255, 255, 255, 1, 160, 10, 1, 168, 10, 1, 176, 10, 0, 208, 10, 1, 168, 11, 0, 176, 11, 0, 200, 11, 1, 208, 11, 0, 216, 11, 0, 224, 11, 1, 232, 11, 1, 240, 11, 1, 216, 12, 0, 232, 12, 1, 128, 13, 5, 136, 13, 1, 146, 13, 0, 160, 13, 135, 128, 128, 15, 168, 13, 135, 128, 128, 15, 192, 13, 1, 200, 13, 0, 216, 13, 0, 128, 14, 0, 141, 14, 205, 204, 140, 63, 152, 14, 0, 160, 14, 128, 128, 128, 4, 184, 14, 1, 216, 14, 0, 224, 14, 0, 232, 14, 255, 255, 255, 255, 255, 255, 255, 255, 127, 128, 15, 0, 136, 15, 1, 152, 15, 1, 176, 15, 0, 184, 15, 1, 192, 15, 0, 208, 15, 1, 216, 15, 15, 224, 15, 0, 232, 15, 1, 240, 15, 0, 248, 15, 0, 128, 16, 0, 136, 16, 0, 146, 16, 5, 1, 2, 8, 7, 3, 152, 16, 1, 160, 16, 95, 170, 16, 0, 176, 16, 0, 200, 16, 160, 141, 6, 216, 16, 0, 224, 16, 0, 232, 16, 0, 128, 17, 1, 136, 17, 0, 144, 17, 0, 168, 17, 0, 192, 17, 1, 216, 17, 100, 224, 17, 0, 232, 17, 0, 248, 17, 0, 128, 18, 0, 144, 18, 0, 152, 18, 0, 168, 18, 16, 176, 18, 3, 192, 18, 0, 224, 18, 1, 232, 18, 0, 128, 19, 1, 136, 19, 0, 152, 19, 1, 160, 19, 128, 2, 178, 19, 0, 184, 19, 16, 192, 19, 0, 216, 19, 0, 229, 19, 205, 204, 204, 61, 232, 19, 0, 240, 19, 5, 152, 20, 32, 160, 20, 1, 184, 20, 10, 192, 20, 30, 200, 20, 0, 208, 20, 0, 216, 20, 32, 234, 20, 0, 240, 20, 0, 248, 20, 0, 128, 21, 1, 136, 21, 0, 152, 21, 255, 255, 255, 255, 255, 255, 255, 255, 255, 1, 160, 21, 0, 168, 21, 1, 176, 21, 1, 184, 21, 0, 192, 21, 0, 200, 21, 0, 216, 21, 0, 224, 21, 0, 232, 21, 0, 240, 21, 0, 248, 21, 0, 136, 22, 0, 144, 22, 0, 152, 22, 0, 160, 22, 1, 170, 22, 19, 10, 13, 99, 104, 117, 110, 107, 95, 112, 114, 101, 112, 95, 117, 115, 18, 2, 45, 49, 170, 22, 22, 10, 16, 99, 104, 117, 110, 107, 95, 115, 105, 122, 101, 95, 98, 121, 116, 101, 115, 18, 2, 45, 49, 170, 22, 19, 10, 13, 103, 112, 117, 115, 95, 112, 101, 114, 95, 110, 111, 100, 101, 18, 2, 45, 49, 170, 22, 23, 10, 17, 110, 99, 99, 108, 95, 111, 112, 95, 108, 97, 117, 110, 99, 104, 95, 117, 115, 18, 2, 45, 49, 170, 22, 20, 10, 14, 110, 105, 99, 95, 115, 112, 101, 101, 100, 95, 103, 98, 112, 115, 18, 2, 45, 49, 170, 22, 12, 10, 6, 114, 116, 116, 95, 117, 115, 18, 2, 45, 49, 176, 22, 0, 184, 22, 1, 208, 22, 1, 216, 22, 0, 232, 22, 0, 240, 22, 1, 128, 23, 0, 144, 23, 0, 160, 23, 0, 176, 23, 0, 184, 23, 1, 192, 23, 1, 202, 23, 0, 208, 23, 135, 128, 128, 15, 216, 23, 0, 224, 23, 0, 232, 23, 1, 240, 23, 1, 250, 23, 0, 128, 24, 0, 144, 24, 0, 152, 24, 0, 160, 24, 0, 176, 24, 1, 184, 24, 20, 192, 24, 40, 200, 24, 0, 208, 24, 1, 216, 24, 0, 224, 24, 0, 242, 24, 1, 1, 152, 25, 0, 160, 25, 2, 176, 25, 0, 186, 25, 0, 192, 25, 0, 208, 25, 0, 216, 25, 0, 224, 25, 0, 232, 25, 0, 136, 26, 40, 144, 26, 20, 152, 26, 0, 168, 26, 0, 32, 1, 40, 1, 48, 1, 74, 9, 8, 1, 16, 1, 26, 3, 10, 1, 0, 98, 1, 0, 146, 1, 1, 0, 152, 1, 1, 184, 1, 1, 200, 1, 29, 40, 255, 255, 255, 255, 255, 255, 255, 255, 255, 1 };
  const int compileOptionsSize = sizeof(compileOptionsData) / sizeof(compileOptionsData[0]);
  const char* compileOptions = reinterpret_cast<const char*>(compileOptionsData);

  PJRT_Client_Compile_Args compile_args;
  compile_args.struct_size = PJRT_Client_Compile_Args_STRUCT_SIZE;
  compile_args.extension_start = nullptr;
  compile_args.client = client_;
  compile_args.program = &program_desc;
  compile_args.compile_options = compileOptions; // No specific compile options for now
  compile_args.compile_options_size = compileOptionsSize;
  // compile_args.executable will be populated

  PJRT_Error* compile_error = context_.pjrtApi_->PJRT_Client_Compile(&compile_args);

  if (compile_error != nullptr) {
    throw context_.convertPjrtErrorToException(compile_error, "PJRT_Client_Compile", __FILE__, __LINE__);
  }

  PJRT_LoadedExecutable *compiledExecutable = compile_args.executable;
  if (!compiledExecutable) {
    throw std::runtime_error("PJRT_Client_Compile reported success, but the executable pointer is null.");
  }

  return LoadedExecutable(context_, compiledExecutable);
}

size_t Client::getNumDevices() const {
  PJRT_Client_AddressableDevices_Args addressableDevicesArgs;
  getAddressableDevices(addressableDevicesArgs);
  return addressableDevicesArgs.num_addressable_devices;
}

DeviceView Client::getDevice(size_t deviceNumber) const {
  PJRT_Client_AddressableDevices_Args addressableDevicesArgs;
  getAddressableDevices(addressableDevicesArgs);

  if (addressableDevicesArgs.num_addressable_devices == 0) {
    throw pjrt::Exception("No addressable devices found.");
  }
  if (deviceNumber >= addressableDevicesArgs.num_addressable_devices) {
    throw pjrt::Exception("Device number is out of range.");
  }

  return DeviceView(context_, addressableDevicesArgs.addressable_devices[deviceNumber]);
}

void Client::getAddressableDevices(PJRT_Client_AddressableDevices_Args &addressableDevicesArgs) const {
  addressableDevicesArgs.struct_size = PJRT_Client_AddressableDevices_Args_STRUCT_SIZE;
  addressableDevicesArgs.extension_start = nullptr;
  addressableDevicesArgs.client = client_;
  // addressableDevicesArgs.addressable_devices and addressableDevicesArgs.num_addressable_devices will be populated

  PJRT_Error* ad_error = context_.pjrtApi_->PJRT_Client_AddressableDevices(&addressableDevicesArgs);
  if (ad_error != nullptr) {
    throw context_.convertPjrtErrorToException(ad_error, "PJRT_Client_AddressableDevices", __FILE__, __LINE__);
  }
}

} // namespace pjrt