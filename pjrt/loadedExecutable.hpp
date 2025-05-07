#ifndef PJRT_LOADED_EXECUTABLE_HPP_
#define PJRT_LOADED_EXECUTABLE_HPP_

#include "buffer.hpp"

struct PJRT_LoadedExecutable;

namespace pjrt {

class Context;
class DeviceView;

class LoadedExecutable {
public:
  LoadedExecutable(const Context &context, PJRT_LoadedExecutable *executable);
  ~LoadedExecutable();

  Buffer execute(const DeviceView &device, const Buffer &inputBuffer);
public:
// private:
  const Context &context_;
  PJRT_LoadedExecutable *executable_;
};
  
} // namespace pjrt

#endif // PJRT_LOADED_EXECUTABLE_HPP_