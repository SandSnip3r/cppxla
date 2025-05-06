#include "device.hpp"

namespace pjrt {
  
Device::Device(const Context &context, PJRT_Device *device) : context_(context), device_(device) {

}

} // namespace pjrt