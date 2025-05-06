#ifndef PJRT_CONTEXT_HPP_
#define PJRT_CONTEXT_HPP_

// Forward declaration.
struct PJRT_Api;

namespace pjrt {

class Context {
public:
  Context();
  ~Context();
public:
// private:
  void *pluginHandle_{nullptr};
  const PJRT_Api *pjrtApi_{nullptr};
};

} // namespace pjrt

#endif // PJRT_CONTEXT_HPP_