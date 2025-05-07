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

#include <stdexcept>

#ifndef PJRT_PLUGIN_PATH
  #error "PJRT_PLUGIN_PATH is not defined. Please define it via CMake's target_compile_definitions."
#endif

namespace pjrt {

Context::Context() {
  const char* plugin_path = PJRT_PLUGIN_PATH;

  // Open the plugin
  // RTLD_LAZY: Resolve symbols only as the code that references them is executed.
  // RTLD_GLOBAL: Make symbols from this library available for symbol resolution of subsequently loaded libraries.
  pluginHandle_ = dlopen(plugin_path, RTLD_LAZY | RTLD_GLOBAL);
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
}

Context::~Context() {
  if (pluginHandle_ != nullptr) {
    // Close the plugin
    if (dlclose(pluginHandle_) != 0) {
      throw std::runtime_error("Error closing PJRT plugin: "+std::string(dlerror()));
    }
  }
}

} // namespace pjrt