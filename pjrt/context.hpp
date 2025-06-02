#ifndef PJRT_CONTEXT_HPP_
#define PJRT_CONTEXT_HPP_

#include "pjrt/exception.hpp"

#include <string_view>

// Forward declaration.
struct PJRT_Api;
struct PJRT_Error;

namespace pjrt {

// This class `dlopen` the PJRT plugin which is specified by CMake.
// Construction can fail, in which case, an exception will be thrown.
// If construction fails, subsequent calls to member functions may also throw.
class Context {
public:
  Context();

  // Attempts to clean up resources, will not throw. If cleanup fails, resources may be leaked.
  ~Context();

  // Cleanup resources held. May throw.
  void destroy();

  int apiMajorVersion() const;
  int apiMinorVersion() const;

  Exception convertPjrtErrorToException(PJRT_Error *error, std::string_view pjrtFunctionName, std::string_view file, int lineNumber) const;

public:
// private:
  void *pluginHandle_{nullptr};
  const PJRT_Api *pjrtApi_{nullptr};
};

} // namespace pjrt

#endif // PJRT_CONTEXT_HPP_