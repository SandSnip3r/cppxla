#ifndef PJRT_LOADED_EXECUTABLE_HPP_
#define PJRT_LOADED_EXECUTABLE_HPP_

#include "buffer.hpp"
// #include "executable.hpp"

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
  // Executable executable_{context_, nullptr};
  size_t numOutputs_;
  std::vector<std::vector<int64_t>> outputDimensions_;
  
private:
  void getExecutableProperties();
};

} // namespace pjrt

#endif // PJRT_LOADED_EXECUTABLE_HPP_