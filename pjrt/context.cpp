#include "context.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif

// Assume pjrt_c_api.h is in the same directory or an include path
#include "pjrt_c_api.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <dlfcn.h> // For dlopen, dlsym, dlclose, dlerror

#include <cassert>
#include <iostream>
#include <stdexcept>

#ifndef PJRT_PLUGIN_PATH
  #error "PJRT_PLUGIN_PATH is not defined. Please define it via CMake's target_compile_definitions."
#endif

namespace pjrt {

Context::Context() {
  const char* pluginPath = PJRT_PLUGIN_PATH;

  // Open the plugin
  // RTLD_LAZY: Resolve symbols only as the code that references them is executed.
  // RTLD_GLOBAL: Make symbols from this library available for symbol resolution of subsequently loaded libraries.
  pluginHandle_ = dlopen(pluginPath, RTLD_LAZY | RTLD_GLOBAL);
  const char *error = dlerror();
  if (error) {
    throw std::runtime_error("Error loading PJRT plugin: "+std::string(error));
  }
  if (!pluginHandle_) {
    throw std::runtime_error("dlopen succeeded, but plugin handle is null");
  }

  // Find the GetPjrtApi symbol
  typedef const PJRT_Api* (*GetPjrtApi_Func_Type)();
  GetPjrtApi_Func_Type GetPjrtApi_func = (GetPjrtApi_Func_Type)dlsym(pluginHandle_, "GetPjrtApi");

  const char* dlsym_error = dlerror();
  if (dlsym_error) {
    throw std::runtime_error("Error finding 'GetPjrtApi' symbol: "+std::string(dlsym_error));
  }
  if (!GetPjrtApi_func) {
    throw std::runtime_error("'GetPjrtApi' symbol found but the function pointer is null.");
  }

  // Call GetPjrtApi to get the API function table
  pjrtApi_ = GetPjrtApi_func();
  if (!pjrtApi_) {
    throw std::runtime_error("Call to GetPjrtApi_func() returned a null PJRT_Api pointer.");
  }

  // Do a quick version check.
  if (pjrtApi_->pjrt_api_version.major_version != PJRT_API_MAJOR) {
    throw pjrt::Exception("PJRT API major version mismatch. Header is " + std::to_string(PJRT_API_MAJOR) + " and shared library is " + std::to_string(pjrtApi_->pjrt_api_version.major_version));
  }
  if (pjrtApi_->pjrt_api_version.minor_version != PJRT_API_MINOR) {
    throw pjrt::Exception("PJRT API minor version mismatch. Header is " + std::to_string(PJRT_API_MINOR) + " and shared library is " + std::to_string(pjrtApi_->pjrt_api_version.minor_version));
  }

  PJRT_Plugin_Initialize_Args pluginInitializeArgs;
  pluginInitializeArgs.struct_size = PJRT_Plugin_Initialize_Args_STRUCT_SIZE;
  pluginInitializeArgs.extension_start = nullptr;
  PJRT_Error* initError = pjrtApi_->PJRT_Plugin_Initialize(&pluginInitializeArgs);
  if (initError != nullptr) {
    throw convertPjrtErrorToException(initError, "PJRT_Plugin_Initialize", __FILE__, __LINE__);
  }
}

Context::~Context() {
  if (pluginHandle_ != nullptr) {
    // Close the plugin
    if (dlclose(pluginHandle_) == 0) {
      // Success
      pluginHandle_ = nullptr;
    } else {
      // dlclose may fail, we choose to do nothing about that failure in the destructor. If you need to handle the failure, call destroy() before destruction.
      std::cerr << "dlclose of plugin failed: \"" << dlerror() << "\"" << std::endl;
    }
  }
}

void Context::destroy() {
  if (pluginHandle_ != nullptr) {
    // Close the plugin
    if (dlclose(pluginHandle_) != 0) {
      throw std::runtime_error("Error closing PJRT plugin: "+std::string(dlerror()));
    }
    pluginHandle_ = nullptr;
  }
}

int Context::apiMajorVersion() const {
  if (pjrtApi_ == nullptr) {
    throw std::runtime_error("Cannot get API major version without Pjrt API");
  }
  return pjrtApi_->pjrt_api_version.major_version;
}

int Context::apiMinorVersion() const {
  if (pjrtApi_ == nullptr) {
    throw std::runtime_error("Cannot get API minor version without Pjrt API");
  }
  return pjrtApi_->pjrt_api_version.minor_version;
}

// For C++20 or newer, replace this with a function which uses std::source_location.
Exception Context::convertPjrtErrorToException(PJRT_Error *error, std::string_view pjrtFunctionName, std::string_view file, int lineNumber) const {
  assert(((void)"Given null error", error != nullptr));

  std::string sourceLocation{pjrtFunctionName};
  sourceLocation += " failed at ";
  sourceLocation += file;
  sourceLocation += ":";
  sourceLocation += std::to_string(lineNumber);
  sourceLocation += ".";

  /* Extract the error message from the API. */
  PJRT_Error_Message_Args error_message_args;
  error_message_args.struct_size = PJRT_Error_Message_Args_STRUCT_SIZE;
  error_message_args.extension_start = nullptr;
  error_message_args.error = error;
  pjrtApi_->PJRT_Error_Message(&error_message_args);

  /* Note that PJRT_Error_Message_Args::message and                                   */
  /* PJRT_Error_Message_Args::message_size's lifetimes match that of the              */
  /* actual PJRT_Error. Once the error is destroyed, we must not access these fields. */
  const std::string errorMessage = sourceLocation + " Error:\n" +
      std::string(error_message_args.message, error_message_args.message_size);

  /* Destroy the PJRT_Error. */
  PJRT_Error_Destroy_Args destroy_error_args;
  destroy_error_args.struct_size = PJRT_Error_Destroy_Args_STRUCT_SIZE;
  destroy_error_args.extension_start = nullptr;
  destroy_error_args.error = error;
  pjrtApi_->PJRT_Error_Destroy(&destroy_error_args);

  return pjrt::Exception(errorMessage);
}

} // namespace pjrt