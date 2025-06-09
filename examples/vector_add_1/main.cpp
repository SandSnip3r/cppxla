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

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  pjrt::Context context;
  std::cout << "Successfully initialized PJRT API." << std::endl << std::endl;

  pjrt::Client client(context);
  std::cout << "PJRT_Client created successfully!" << std::endl;
  std::cout << "Platform Name: " << client.platformName() << std::endl << std::endl;
  pjrt::DeviceView device = client.getFirstDevice();

  // Read HLO program from file
  std::string stableHloStr;
  try {
    const char* hloProgramFilePath = "myStableHlo.txt";
    stableHloStr = readFileIntoStream(hloProgramFilePath);
    std::cout << "Successfully read HLO program from: " << hloProgramFilePath << std::endl << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Error reading HLO file: " << e.what() << std::endl;
    return 1;
  }

  // Compile program
  pjrt::LoadedExecutable loadedExecutable = client.compileFromStableHloString(stableHloStr);

  // Try running the compiled executable
  std::cout << "Creating input buffer..." << std::endl;
  std::vector<float> host_input_data(128, 0.0f);
  std::vector<int64_t> shape = {128};
  std::future<pjrt::Buffer> inputBuffer = client.transferToDevice(host_input_data.data(), shape, device);
  inputBuffer.wait();
  std::cout << "Input buffer created and transfer to device is complete." << std::endl << std::endl;

  std::cout << "Executing compiled program..." << std::endl;
  std::future<pjrt::Buffer> outputBuffer = loadedExecutable.execute(device, inputBuffer.get());
  outputBuffer.wait();
  std::cout << "Execution complete" << std::endl << std::endl;

  std::cout << "Transferring result to host" << std::endl;
  std::future<float> outputFuture = outputBuffer.get().toHost<float>();
  float result = outputFuture.get();
  std::cout << "Output value: " << result << std::endl;

  return 0;
}