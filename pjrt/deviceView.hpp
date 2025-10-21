#ifndef PJRT_DEVICE_VIEW_HPP_
#define PJRT_DEVICE_VIEW_HPP_

#include <string>

struct PJRT_Device;

namespace pjrt {

class Context;

class DeviceView {
public:
  DeviceView(const Context &context, PJRT_Device *device);
  DeviceView(DeviceView &&other);
  DeviceView& operator=(DeviceView &&other);
  // No destructor because PJRT_Client owns the devices and no cleanup is required in this class.

  std::string description() const;
public:
// private:
  const Context &context_;
  PJRT_Device *device_{nullptr};
};

} // namespace pjrt

#endif // PJRT_DEVICE_VIEW_HPP_