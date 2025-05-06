#ifndef PJRT_CLIENT_HPP_
#define PJRT_CLIENT_HPP_

#include "buffer.hpp"
#include "device.hpp"
#include "loadedExecutable.hpp"

#include <string>

struct PJRT_Client;

namespace pjrt {

class Context;

class Client {
public:
  Client(const Context &context);
  ~Client();

  std::string platformName() const;

  LoadedExecutable compileFromStableHloString(const std::string &stableHloProgram) const;
  Device getFirstDevice() const;

  // Synchronously creates buffer and transfers it to device.
  Buffer createBufferFromData(float singleFloat, const Device &device) const;
public:
// private:
  const Context &context_;
  PJRT_Client *client_{nullptr};
};
  
} // namespace pjrt

#endif // PJRT_CLIENT_HPP_