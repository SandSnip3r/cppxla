#include "buffer.hpp"
#include "common.hpp"
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

} // namespace pjrt
