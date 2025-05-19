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

void executeAndVerify(pjrt::DeviceView &device, pjrt::Client &client, pjrt::LoadedExecutable &executable, float input) {
  std::future<pjrt::Buffer> inputBuffer = client.createBufferFromData(input, device);
  std::cout << "Started transfer" << std::endl;
  pjrt::Buffer buffer = inputBuffer.get();
  std::cout << "Transfer complete, starting execution" << std::endl;
  std::future<pjrt::Buffer> outputBuffer = executable.execute(device, buffer);
  pjrt::Buffer buffer2 = outputBuffer.get();
  std::cout << "Execution complete, transferring to host" << std::endl;
  std::future<float> outputFuture = buffer2.toHost();
  float output = outputFuture.get();
  if (output != input+1) {
    std::cout << "Unexpected result! " << input+1 << " expected, " << output << " received" << std::endl;
  } else {
    std::cout << "All good. " << input << "+1 = " << output << std::endl;
  }
  std::cout << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " <path to program in StableHlo text format>" << std::endl;
    return 1;
  }
  pjrt::Context context;
  std::cout << "Successfully initialized PJRT API." << std::endl;
  performVersionCheck(context);

  pjrt::Client client(context);
  std::cout << "PJRT_Client created successfully!" << std::endl;
  std::cout << "Platform Name: " << client.platformName() << std::endl;

  // Read HLO program from file
  std::string stableHloStr;
  try {
    const char* hloProgramFilePath = argv[1];
    stableHloStr = readFileIntoStream(hloProgramFilePath);
    std::cout << "Successfully read HLO program from: " << hloProgramFilePath << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Error reading HLO file: " << e.what() << std::endl;
    return 1;
  }

  // Compile program
  pjrt::LoadedExecutable executable = client.compileFromStableHloString(stableHloStr);
  pjrt::DeviceView device = client.getFirstDevice();
  std::cout << "Created device" << std::endl;

  // Try running the compiled executable
  std::cout << "Creating input buffer..." << std::endl;
  float host_input_data = 41.0f;
  std::future<pjrt::Buffer> inputBuffer = client.createBufferFromData(host_input_data, device);
  std::cout << "Input buffer created and transfer to device is complete." << std::endl;
  std::cout << "Executing compiled program..." << std::endl;
  std::future<pjrt::Buffer> outputBuffer = executable.execute(device, inputBuffer.get());
  std::cout << "Execution complete" << std::endl;
  std::future<float> outputFuture = outputBuffer.get().toHost();
  float result = outputFuture.get();
  std::cout << "Output value: " << result << std::endl;

  // Run the program a few more times on some different inputs and verify that the output is correct.
  for (int i=0; i<10; ++i) {
    executeAndVerify(device, client, executable, i);
  }
  return 0;
}