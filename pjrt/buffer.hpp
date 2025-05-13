#ifndef PJRT_BUFFER_HPP_
#define PJRT_BUFFER_HPP_

#include <future>

struct PJRT_Buffer;

namespace pjrt {

class Context;

class Buffer {
public:
  Buffer(const Context &context, PJRT_Buffer *buffer);
  ~Buffer();

  std::future<float> toHost();
public:
// private:
  const Context &context_;
  PJRT_Buffer *buffer_{nullptr};
};
  
} // namespace pjrt

#endif // PJRT_BUFFER_HPP_