#include "buffer.hpp"
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

#include <stdexcept>

namespace pjrt {

Buffer::Buffer(const Context &context, PJRT_Buffer *buffer) : context_(context), buffer_(buffer) {

}

Buffer::~Buffer() {
  if (buffer_) {
    PJRT_Buffer_Destroy_Args destroy_args;
    destroy_args.struct_size = PJRT_Buffer_Destroy_Args_STRUCT_SIZE;
    destroy_args.extension_start = nullptr;
    destroy_args.buffer = buffer_;
    PJRT_Error* error = context_.pjrtApi_->PJRT_Buffer_Destroy(&destroy_args);
    if (error) {
      throw std::runtime_error(freeErrorAndReturnString(context_, error, "Failed to destroy buffer"));
    }
  }
}

float Buffer::toHost() {
  float hostOutputData = 0.0f; // To store the scalar f32 result

  PJRT_Buffer_ToHostBuffer_Args bthh_args;
  bthh_args.struct_size = PJRT_Buffer_ToHostBuffer_Args_STRUCT_SIZE;
  bthh_args.extension_start = nullptr;
  bthh_args.src = buffer_;
  bthh_args.dst = &hostOutputData;
  bthh_args.dst_size = sizeof(float); // Size of our host buffer
  bthh_args.host_layout = nullptr; // Use default/source layout
  // bthh_args.event will be populated

  PJRT_Error* bthh_error = context_.pjrtApi_->PJRT_Buffer_ToHostBuffer(&bthh_args);
  if (bthh_error != nullptr) {
    throw std::runtime_error(freeErrorAndReturnString(context_, bthh_error, "PJRT_Buffer_ToHostBuffer failed."));
  }
  
  Event transferCompletedEvent(context_, bthh_args.event);
  transferCompletedEvent.wait();

  return hostOutputData;
}

} // namespace pjrt
