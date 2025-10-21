#include "deviceView.hpp"
#include "context.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif

#include "pjrt_c_api.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <cassert>

namespace pjrt {

DeviceView::DeviceView(const Context &context, PJRT_Device *device) : context_(context), device_(device) {

}

DeviceView::DeviceView(DeviceView &&other) : context_(other.context_), device_(other.device_) {
  other.device_ = nullptr;
}

DeviceView& DeviceView::operator=(DeviceView &&other) {
  assert(((void)"Cannot assign a DeviceView from one context to another", &other.context_ == &context_));
  if (this == &other) {
    return *this;
  }
  device_ = other.device_;
  other.device_ = nullptr;
  return *this;
}

std::string DeviceView::description() const {
  PJRT_Device_GetDescription_Args get_desc_args;
  get_desc_args.struct_size = PJRT_Device_GetDescription_Args_STRUCT_SIZE;
  get_desc_args.extension_start = nullptr;
  get_desc_args.device = device_;
  PJRT_Error* error = context_.pjrtApi_->PJRT_Device_GetDescription(&get_desc_args);
  if (error != nullptr) {
    throw context_.convertPjrtErrorToException(error, "PJRT_Device_GetDescription", __FILE__, __LINE__);
  }

  PJRT_DeviceDescription_ToString_Args to_string_args;
  to_string_args.struct_size = PJRT_DeviceDescription_ToString_Args_STRUCT_SIZE;
  to_string_args.extension_start = nullptr;
  to_string_args.device_description = get_desc_args.device_description;
  error = context_.pjrtApi_->PJRT_DeviceDescription_ToString(&to_string_args);
  if (error != nullptr) {
    throw context_.convertPjrtErrorToException(error, "PJRT_DeviceDescription_ToString", __FILE__, __LINE__);
  }
  return std::string(to_string_args.to_string, to_string_args.to_string_size);
}

} // namespace pjrt