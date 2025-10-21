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
  LoadedExecutable(LoadedExecutable &&other);
  LoadedExecutable& operator=(LoadedExecutable &&other);
  ~LoadedExecutable();

  void destroy();

  std::future<std::vector<Buffer>> execute(const DeviceView& device,
                                           std::vector<Buffer*>& argument_handles);
public:
// private:
  const Context &context_;
  PJRT_LoadedExecutable *loadedExecutable_;
  
private:
  Executable getExecutable() const;
};

} // namespace pjrt

#endif // PJRT_LOADED_EXECUTABLE_HPP_