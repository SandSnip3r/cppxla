#ifndef PJRT_COMMON_HPP_
#define PJRT_COMMON_HPP_

// #include "context.hpp"

#include <string>
#include <string_view>

struct PJRT_Error;

namespace pjrt {

class Context;

// Frees the PJRT_Error and returns a std::string of the error.
std::string freeErrorAndReturnString(const Context &context, PJRT_Error *error, const std::string &contextMessage);

} // namespace pjrt

#endif // PJRT_COMMON_HPP_