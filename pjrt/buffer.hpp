#ifndef PJRT_BUFFER_HPP_
#define PJRT_BUFFER_HPP_

#include "pjrt/context.hpp"
#include "pjrt/detail/callbackUserData.hpp"
#include "pjrt/event.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif
// Assume pjrt_c_api.h is in the same directory or an include path
#include "pjrt_c_api.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <cassert>
#include <cstdint>
#include <future>
#include <iostream>
#include <optional>
#include <vector>

struct PJRT_Buffer;

namespace pjrt {

class Buffer {
public:
  Buffer(const Context &context);
  Buffer(const Context &context, PJRT_Buffer *buffer, const std::vector<int64_t> &dims);
  Buffer(Buffer &&other);

  Buffer& operator=(Buffer &&other);

  const std::vector<int64_t>& dimensions() const { return dimensions_; }
  PJRT_Buffer* c_buffer() const { return buffer_; }

  // Attempts to clean up resources, will not throw. If cleanup fails, resources may be leaked.
  ~Buffer();

  // Cleanup resources held. May throw.
  void destroy();

  template<typename T>
  std::future<std::vector<T>> toHost() {
    // First, query the API to check the required size of the output.
    PJRT_Buffer_ToHostBuffer_Args bthh_args;
    bthh_args.struct_size = PJRT_Buffer_ToHostBuffer_Args_STRUCT_SIZE;
    bthh_args.extension_start = nullptr;
    bthh_args.src = buffer_;
    bthh_args.dst = nullptr;
    // bthh_args.dst_size will be populated.
    bthh_args.host_layout = nullptr; // Use default/source layout
    bthh_args.event = nullptr;

    PJRT_Error* pjrtError = context_.pjrtApi_->PJRT_Buffer_ToHostBuffer(&bthh_args);
    if (pjrtError != nullptr) {
      // TODO: The PJRT documentation does not say whether or not we need to free the event in the args struct in the case of an error. My current guess is that we do not.
      throw context_.convertPjrtErrorToException(pjrtError, "PJRT_Buffer_ToHostBuffer", __FILE__, __LINE__);
    }

    assert(((void)"There should be no event when simply querying size", bthh_args.event == nullptr));
    
    auto callbackUserData = std::make_unique<detail::CallbackUserData<std::vector<T>>>(context_);
    std::vector<T> &hostData = callbackUserData->getData();
    hostData.resize(bthh_args.dst_size / sizeof(T));
    bthh_args.dst = hostData.data();

    pjrtError = context_.pjrtApi_->PJRT_Buffer_ToHostBuffer(&bthh_args);
    if (pjrtError != nullptr) {
      // TODO: The PJRT documentation does not say whether or not we need to free the event in the args struct in the case of an error. My current guess is that we do not.
      throw context_.convertPjrtErrorToException(pjrtError, "PJRT_Buffer_ToHostBuffer", __FILE__, __LINE__);
    }
    
    return context_.getFutureForEvent(bthh_args.event, std::move(callbackUserData));
  }
private:
  const Context &context_;
  PJRT_Buffer *buffer_{nullptr};
  const std::vector<int64_t> dimensions_;

  std::optional<pjrt::Exception> privateDestroyBuffer();
};

} // namespace pjrt

#endif // PJRT_BUFFER_HPP_