#ifndef PJRT_EXECUTABLE_HPP_
#define PJRT_EXECUTABLE_HPP_

#include <cstddef>
#include <vector>

struct PJRT_Executable;

namespace pjrt {

class Context;

class Executable {
public:
  Executable(const Context &context, PJRT_Executable *executable);
  Executable(Executable &&other);
  Executable& operator=(Executable &&other);
  ~Executable();

  void destroy();

  size_t getNumOutputs() const;
  std::vector<std::vector<int64_t>> getOutputDimensions() const;
private:
  const Context &context_;
  PJRT_Executable *executable_;
};

} // namespace pjrt

#endif // PJRT_EXECUTABLE_HPP_