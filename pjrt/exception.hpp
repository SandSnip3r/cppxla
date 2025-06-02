#ifndef PJRT_EXCEPTION_HPP_
#define PJRT_EXCEPTION_HPP_

#include <exception>
#include <string>

namespace pjrt {

/**
 * @class Exception
 * @brief Base class for all exceptions thrown by pjrt.
 *
 * This allows users to catch all library-specific exceptions
 * with a single catch block.
 */
class Exception : public std::exception {
public:
  /**
   * @brief Constructs the exception with an explanatory string.
   * @param message The message to be returned by what().
   */
  explicit Exception(std::string message);

  /**
   * @brief Returns the explanatory string.
   *
   * This function is marked noexcept as it's an override from std::exception.
   * @return A null-terminated string with the exception's message.
   */
  const char* what() const noexcept override;

protected:
  std::string msg_;
};

} // namespace pjrt

#endif // PJRT_EXCEPTION_HPP_