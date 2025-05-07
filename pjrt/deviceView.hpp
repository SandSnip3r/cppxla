#ifndef PJRT_DEVICE_VIEW_HPP_
#define PJRT_DEVICE_VIEW_HPP_

struct PJRT_Device;

namespace pjrt {

class Context;

class DeviceView {
public:
  DeviceView(const Context &context, PJRT_Device *device);
  // No destructor because PJRT_Client owns the devices and no cleanup is required.
public:
// private:
  const Context &context_;
  PJRT_Device *device_{nullptr};
};

} // namespace pjrt

#endif // PJRT_DEVICE_VIEW_HPP_