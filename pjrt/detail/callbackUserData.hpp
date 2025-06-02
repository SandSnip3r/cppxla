#ifndef PJRT_DETAIL_CALLBACK_USER_DATA_HPP_
#define PJRT_DETAIL_CALLBACK_USER_DATA_HPP_

#include "pjrt/context.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif

// Assume pjrt_c_api.h is in the same directory or an include path
#include "pjrt_c_api.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <cassert>
#include <future>
#include <iostream>

namespace pjrt::detail {

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


template <typename DataType>
void eventReadyCallback(PJRT_Error *error, void *userArgment) {
  assert(((void)"User argument is null", userArgment != nullptr));

  CallbackUserData<DataType> *callbackUserData = static_cast<CallbackUserData<DataType>*>(userArgment);
  if (error == nullptr) {
    // The event has completed without error, fulfill the promise.
    callbackUserData->fulfill();
  } else {
    // An error occurred.
    callbackUserData->setException(std::make_exception_ptr(callbackUserData->getContext().convertPjrtErrorToException(error, "PJRT error when calling user-provided callback", __FILE__, __LINE__)));
  }

  // Free the data.
  delete callbackUserData;
}

template <typename DataType>
std::future<DataType> getFutureForEvent(const Context &context, PJRT_Event *event, std::unique_ptr<detail::CallbackUserData<DataType>> &&callbackUserData) {
  std::future<DataType> future = callbackUserData->getFuture();
  {
    PJRT_Event_OnReady_Args eventOnReadyArgs;
    eventOnReadyArgs.struct_size = PJRT_Event_OnReady_Args_STRUCT_SIZE;
    eventOnReadyArgs.extension_start = nullptr;
    eventOnReadyArgs.event = event;
    eventOnReadyArgs.callback = &detail::eventReadyCallback<DataType>;
    // Pass ownership to PJRT. It will come back to us in our callback or we'll free it momentarily in the case of an error.
    detail::CallbackUserData<DataType> *callbackUserDataRawPtr = callbackUserData.release();
    eventOnReadyArgs.user_arg = callbackUserDataRawPtr;

    PJRT_Error *eventReadyError = context.pjrtApi_->PJRT_Event_OnReady(&eventOnReadyArgs);
    if (eventReadyError != nullptr) {
      // TODO: Are we responsible for freeing our CallbackUserData? My current guess is that we are.
      delete callbackUserDataRawPtr;
      throw context.convertPjrtErrorToException(eventReadyError, "PJRT_Event_OnReady", __FILE__, __LINE__);
    }
  }
  return future;
}

} // namespace pjrt::detail

#endif // PJRT_DETAIL_CALLBACK_USER_DATA_HPP_