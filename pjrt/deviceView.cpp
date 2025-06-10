#include "deviceView.hpp"

#include <cassert>

namespace pjrt {

DeviceView::DeviceView(const Context &context, PJRT_Device *device) : context_(context), device_(device) {

}

DeviceView::DeviceView(DeviceView &&other) : context_(other.context_), device_(other.device_) {

}

DeviceView& DeviceView::operator=(DeviceView &&other) {
  assert(((void)"Cannot assign a DeviceView from one context to another", &other.context_ == &context_));
  device_ = other.device_;
  return *this;
}

} // namespace pjrt