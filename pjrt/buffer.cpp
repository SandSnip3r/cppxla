#include "buffer.hpp"
#include "context.hpp"
#include "detail/callbackUserData.hpp"
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

#include <stdexcept>

namespace pjrt {

Buffer::Buffer(const Context &context) : context_(context) {}

Buffer::Buffer(const Context &context, PJRT_Buffer *buffer) : context_(context), buffer_(buffer) {}

Buffer::Buffer(Buffer &&other) : context_(other.context_), buffer_(other.buffer_) {
  // Set source's buffer to nullptr so that it does not try to free that resource on destruction.
  other.buffer_ = nullptr;
}

Buffer::~Buffer() {
  if (buffer_ == nullptr) {
    return;
  }
  PJRT_Buffer_Destroy_Args destroy_args;
  destroy_args.struct_size = PJRT_Buffer_Destroy_Args_STRUCT_SIZE;
  destroy_args.extension_start = nullptr;
  destroy_args.buffer = buffer_;
  PJRT_Error* pjrtError = context_.pjrtApi_->PJRT_Buffer_Destroy(&destroy_args);
  if (pjrtError == nullptr) {
    return;
  }
  // PJRT_Buffer_Destroy may fail, we choose to do nothing about that failure in the destructor. If you need to handle the failure, call destroy() before destruction.
  const pjrt::Exception ex = context_.convertPjrtErrorToException(pjrtError, "PJRT_Buffer_Destroy", __FILE__, __LINE__);
  std::cerr << "pjrt::Buffer destructor failed to destroy PJRT_Buffer: \"" << ex.what() << "\"" << std::endl;
}

void Buffer::destroy() {
  if (buffer_ == nullptr) {
    return;
  }
  PJRT_Buffer_Destroy_Args destroy_args;
  destroy_args.struct_size = PJRT_Buffer_Destroy_Args_STRUCT_SIZE;
  destroy_args.extension_start = nullptr;
  destroy_args.buffer = buffer_;
  PJRT_Error* pjrtError = context_.pjrtApi_->PJRT_Buffer_Destroy(&destroy_args);
  if (pjrtError == nullptr) {
    return;
  }
  throw context_.convertPjrtErrorToException(pjrtError, "PJRT_Buffer_Destroy", __FILE__, __LINE__);
}

std::future<float> Buffer::toHost() {
  std::unique_ptr<detail::CallbackUserData<float>> callbackUserData = std::make_unique<detail::CallbackUserData<float>>(context_);

  PJRT_Buffer_ToHostBuffer_Args bthh_args;
  bthh_args.struct_size = PJRT_Buffer_ToHostBuffer_Args_STRUCT_SIZE;
  bthh_args.extension_start = nullptr;
  bthh_args.src = buffer_;
  bthh_args.dst = &callbackUserData->getData();
  bthh_args.dst_size = sizeof(float); // Size of our host buffer
  bthh_args.host_layout = nullptr; // Use default/source layout
  // bthh_args.event will be populated

  PJRT_Error* pjrtError = context_.pjrtApi_->PJRT_Buffer_ToHostBuffer(&bthh_args);
  if (pjrtError != nullptr) {
    throw context_.convertPjrtErrorToException(pjrtError, "PJRT_Buffer_ToHostBuffer", __FILE__, __LINE__);
  }
  // TODO: The PJRT documentation does not say whether or not we need to free the event in the case of an error. My current guess is that we do not.

  return getFutureForEvent(context_, bthh_args.event, std::move(callbackUserData));
}

} // namespace pjrt
