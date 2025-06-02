#ifndef PJRT_BUFFER_HPP_
#define PJRT_BUFFER_HPP_

#include <future>

struct PJRT_Buffer;

namespace pjrt {

class Context;

class Buffer {
public:
  Buffer(const Context &context);
  Buffer(const Context &context, PJRT_Buffer *buffer);
  Buffer(Buffer &&other);

  // Attempts to clean up resources, will not throw. If cleanup fails, resources may be leaked.
  ~Buffer();

  // Cleanup resources held. May throw.
  void destroy();

  std::future<float> toHost();
public:
// private:
  const Context &context_;
  PJRT_Buffer *buffer_{nullptr};
};

} // namespace pjrt

#endif // PJRT_BUFFER_HPP_