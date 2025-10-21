#ifndef PJRT_CLIENT_HPP_
#define PJRT_CLIENT_HPP_

#include "buffer.hpp"
#include "detail/callbackUserData.hpp"
#include "detail/types.hpp"
#include "deviceView.hpp"
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

#include <future>
#include <string>
#include <vector>

struct PJRT_Client;

namespace pjrt {

class Context;

class Client {
public:
  Client(const Context &context);
  ~Client();

  void destroy();

  std::string platformName() const;

  LoadedExecutable compileFromStableHloString(const std::string &stableHloProgram) const;
  size_t getNumDevices() const;
  DeviceView getDevice(size_t deviceNumber) const;

  // Asynchronously transfers given data to the specified device.
  // `shape` must stay alive until the future is ready.
  template <typename T>
  std::future<Buffer> transferToDevice(T *data, const std::vector<int64_t> &shape, const DeviceView &device) const;
public:
// private:
  const Context &context_;
  PJRT_Client *client_{nullptr};

private:
  void getAddressableDevices(PJRT_Client_AddressableDevices_Args &addressableDevicesArgs) const;
};

template <typename T>
std::future<Buffer> Client::transferToDevice(T *data, const std::vector<int64_t> &shape, const DeviceView &device) const {
  // Create Input Buffer from Host Data
  PJRT_Client_BufferFromHostBuffer_Args bfhh_args;
  bfhh_args.struct_size = PJRT_Client_BufferFromHostBuffer_Args_STRUCT_SIZE;
  bfhh_args.extension_start = nullptr;
  bfhh_args.client = client_;
  bfhh_args.data = data;
  bfhh_args.type = detail::TypeToPjrtBufferType<T>();

  if (shape.empty() && std::is_arithmetic_v<T>) { // Handle scalar specifically if dimensions is empty
    bfhh_args.dims = nullptr;
    bfhh_args.num_dims = 0; // PJRT typically represents scalars as rank-0 tensors
  } else {
    bfhh_args.dims = shape.data();
    bfhh_args.num_dims = shape.size();
  }

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
    throw context_.convertPjrtErrorToException(bfhh_error, "PJRT_Client_BufferFromHostBuffer", __FILE__, __LINE__);
  }

  std::unique_ptr<detail::CallbackUserData<Buffer>> callbackUserData = std::make_unique<detail::CallbackUserData<Buffer>>(context_, Buffer(context_, bfhh_args.buffer, shape));
  return context_.getFutureForEvent(bfhh_args.done_with_host_buffer, std::move(callbackUserData));
}

} // namespace pjrt

#endif // PJRT_CLIENT_HPP_