#ifndef PJRT_LOADED_EXECUTABLE_HPP_
#define PJRT_LOADED_EXECUTABLE_HPP_

#include "buffer.hpp"
#include "executable.hpp"

#include <future>

struct PJRT_LoadedExecutable;

namespace pjrt {

class Context;
class DeviceView;

class LoadedExecutable {
public:
  LoadedExecutable(const Context &context, PJRT_LoadedExecutable *loadedExecutable);
  ~LoadedExecutable();

  void destroy();

  std::future<Buffer> execute(const DeviceView &device, const Buffer &inputBuffer);
public:
// private:
  const Context &context_;
  PJRT_LoadedExecutable *loadedExecutable_;
  Executable executable_{context_, nullptr};

};

} // namespace pjrt

#endif // PJRT_LOADED_EXECUTABLE_HPP_