#ifndef PJRT_DETAIL_CALLBACK_USER_DATA_HPP_
#define PJRT_DETAIL_CALLBACK_USER_DATA_HPP_

// #include "pjrt/context.hpp"

// #if defined(__GNUC__) || defined(__clang__)
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wchanges-meaning"
// #endif

// // Assume pjrt_c_api.h is in the same directory or an include path
// #include "pjrt_c_api.h"

// #if defined(__GNUC__) || defined(__clang__)
// #pragma GCC diagnostic pop
// #endif

#include <cassert>
#include <future>
#include <iostream>

namespace pjrt {

class Context;

namespace detail {

template <typename DataType>
class CallbackUserData {
public:
  CallbackUserData(const Context &context) : context_(context) {}
  CallbackUserData(const Context &context, DataType &&data) : context_(context), data_(std::move(data)) {}

  const Context &getContext() const { return context_; }

  std::future<DataType> getFuture() {
    return promise_.get_future();
  }

  DataType& getData() {
    return data_;
  }

  void setException(std::exception_ptr &&exceptionPtr) {
    promise_.set_exception(std::move(exceptionPtr));
  }

  void fulfill() {
    promise_.set_value(std::move(data_));
    // TODO: Maybe throw errors if anything else references data_ after this.
  }
private:
  const Context &context_;
  std::promise<DataType> promise_;
  DataType data_;
};

} // namespace detail
} // namespace pjrt

#endif // PJRT_DETAIL_CALLBACK_USER_DATA_HPP_