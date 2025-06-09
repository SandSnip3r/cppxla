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

} // namespace pjrt
