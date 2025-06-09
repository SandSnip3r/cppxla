#ifndef PJRT_CONTEXT_HPP_
#define PJRT_CONTEXT_HPP_

#include "pjrt/exception.hpp"
#include "pjrt/detail/callbackUserData.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif

// Assume pjrt_c_api.h is in the same directory or an include path
#include "pjrt_c_api.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <future>
#include <string_view>

// Forward declaration.
struct PJRT_Api;
struct PJRT_Error;
struct PJRT_Event;

namespace pjrt {

template <typename DataType>
void eventReadyCallback(PJRT_Error *error, void *userArgment);

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

  template <typename DataType>
  std::future<DataType> getFutureForEvent(PJRT_Event *event, std::unique_ptr<detail::CallbackUserData<DataType>> &&callbackUserData) const {
    std::future<DataType> future = callbackUserData->getFuture();
    {
      PJRT_Event_OnReady_Args eventOnReadyArgs;
      eventOnReadyArgs.struct_size = PJRT_Event_OnReady_Args_STRUCT_SIZE;
      eventOnReadyArgs.extension_start = nullptr;
      eventOnReadyArgs.event = event;
      eventOnReadyArgs.callback = &eventReadyCallback<DataType>;
      // Pass ownership to PJRT. It will come back to us in our callback or we'll free it momentarily in the case of an error.
      detail::CallbackUserData<DataType> *callbackUserDataRawPtr = callbackUserData.release();
      eventOnReadyArgs.user_arg = callbackUserDataRawPtr;

      PJRT_Error *eventReadyError = pjrtApi_->PJRT_Event_OnReady(&eventOnReadyArgs);
      if (eventReadyError != nullptr) {
        // TODO: Are we responsible for freeing our CallbackUserData? My current guess is that we are.
        delete callbackUserDataRawPtr;
        throw convertPjrtErrorToException(eventReadyError, "PJRT_Event_OnReady", __FILE__, __LINE__);
      }
    }
    return future;
  }

public:
// private:
  void *pluginHandle_{nullptr};
  const PJRT_Api *pjrtApi_{nullptr};
};

template <typename DataType>
void eventReadyCallback(PJRT_Error *error, void *userArgment) {
  assert(((void)"User argument is null", userArgment != nullptr));

  detail::CallbackUserData<DataType> *callbackUserData = static_cast<detail::CallbackUserData<DataType>*>(userArgment);
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

} // namespace pjrt

#endif // PJRT_CONTEXT_HPP_