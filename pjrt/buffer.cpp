#include "buffer.hpp"
#include "common.hpp"
#include "context.hpp"
#include "event.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif

// Assume pjrt_c_api.h is in the same directory or an include path
#include "pjrt_c_api.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <iostream>
#include <stdexcept>

namespace pjrt {

Buffer::Buffer(const Context &context, PJRT_Buffer *buffer) : context_(context), buffer_(buffer) {

}

Buffer::~Buffer() {
  if (buffer_) {
    PJRT_Buffer_Destroy_Args destroy_args;
    destroy_args.struct_size = PJRT_Buffer_Destroy_Args_STRUCT_SIZE;
    destroy_args.extension_start = nullptr;
    destroy_args.buffer = buffer_;
    PJRT_Error* error = context_.pjrtApi_->PJRT_Buffer_Destroy(&destroy_args);
    if (error) {
      throw std::runtime_error(freeErrorAndReturnString(context_, error, "Failed to destroy buffer"));
    }
  }
}

class CallbackUserData {
public:
  CallbackUserData(const Context &context) : context_(context) {}

  const Context &getContext() const { return context_; }

  std::future<float> getFuture() {
    return promise_.get_future();
  }

  float* getDataPtr() {
    return &result_;
  }

  void setException(std::exception_ptr &&exceptionPtr) {
    promise_.set_exception(std::move(exceptionPtr));
  }

  void fulfill() {
    promise_.set_value(result_);
  }
private:
  const Context &context_;
  std::promise<float> promise_;
  float result_;
};

void eventReadyCallback(PJRT_Error *error, void *userArgment) {
  if (userArgment != nullptr) {
    CallbackUserData *callbackUserData = static_cast<CallbackUserData*>(userArgment);

    if (error != nullptr) {
      callbackUserData->setException(std::make_exception_ptr(std::runtime_error(freeErrorAndReturnString(callbackUserData->getContext(), error, "PJRT error when calling user-provided callback"))));
    } else {
      // The event has completed, fulfill the promise.
      callbackUserData->fulfill();
    }

    // Free the data.
    delete callbackUserData;
  } else {
    std::cout << "Given null user argument!" << std::endl;
    if (error != nullptr) {
      // We have no way to get the error back to the user.
      throw std::runtime_error("PJRT_Event_OnReady called our callback with an error, but did not provide us with our user argument");
    }
  }
}

std::future<float> Buffer::toHost() {
  std::unique_ptr<CallbackUserData> callbackUserData = std::make_unique<CallbackUserData>(context_);

  PJRT_Buffer_ToHostBuffer_Args bthh_args;
  bthh_args.struct_size = PJRT_Buffer_ToHostBuffer_Args_STRUCT_SIZE;
  bthh_args.extension_start = nullptr;
  bthh_args.src = buffer_;
  bthh_args.dst = callbackUserData->getDataPtr();
  bthh_args.dst_size = sizeof(float); // Size of our host buffer
  bthh_args.host_layout = nullptr; // Use default/source layout
  // bthh_args.event will be populated

  PJRT_Error* bthh_error = context_.pjrtApi_->PJRT_Buffer_ToHostBuffer(&bthh_args);
  if (bthh_error != nullptr) {
    // TODO: The PJRT documentation does not say whether or not we need to free the event in the case of an error. My current guess is that we do not.
    throw std::runtime_error(freeErrorAndReturnString(context_, bthh_error, "PJRT_Buffer_ToHostBuffer failed."));
  }

  std::future<float> future = callbackUserData->getFuture();

  {
    PJRT_Event_OnReady_Args eventOnReadyArgs;
    eventOnReadyArgs.struct_size = PJRT_Event_OnReady_Args_STRUCT_SIZE;
    eventOnReadyArgs.extension_start = nullptr;
    eventOnReadyArgs.event = bthh_args.event;
    eventOnReadyArgs.callback = &eventReadyCallback;
    // Pass ownership to PJRT. It will come back to us in our callback or we'll free it momentarily in the case of an error.
    CallbackUserData *callbackUserDataRawPtr = callbackUserData.release();
    eventOnReadyArgs.user_arg = callbackUserDataRawPtr;

    PJRT_Error *eventReadyError = context_.pjrtApi_->PJRT_Event_OnReady(&eventOnReadyArgs);
    if (eventReadyError != nullptr) {
      // TODO: Are we responsible for freeing our CallbackUserData? My current guess is that we are.
      delete callbackUserDataRawPtr;
      throw std::runtime_error(freeErrorAndReturnString(context_, eventReadyError, "PJRT_Event_OnReady failed."));
    }
  }

  return future;
}

} // namespace pjrt
