#include "pjrt/exception.hpp"

namespace pjrt {

Exception::Exception(std::string message) : msg_(std::move(message)) {}

const char* Exception::what() const noexcept {
  return msg_.c_str();
}

} // namespace pjrt