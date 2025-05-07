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

#include <stdexcept>

namespace pjrt {

Event::Event(const Context &context, PJRT_Event *event) : context_(context), event_(event) {}

Event::~Event() {
  if (event_) {
    destroyEvent();
  }
}
  
void Event::wait() {
  if (event_) {
    PJRT_Event_Await_Args await_args;
    await_args.struct_size = PJRT_Event_Await_Args_STRUCT_SIZE;
    await_args.extension_start = nullptr;
    await_args.event = event_;
    PJRT_Error* await_error = context_.pjrtApi_->PJRT_Event_Await(&await_args);
    if (await_error) {
      throw std::runtime_error(freeErrorAndReturnString(context_, await_error, "PJRT_Event_Await failed."));
    }

    // Check the status of the execution from the event itself
    PJRT_Event_Error_Args event_error_args;
    event_error_args.struct_size = PJRT_Event_Error_Args_STRUCT_SIZE;
    event_error_args.extension_start = nullptr;
    event_error_args.event = event_;
    PJRT_Error* exec_status_error = context_.pjrtApi_->PJRT_Event_Error(&event_error_args); // This gets the error *from* the event
    if (exec_status_error) {
      throw std::runtime_error(freeErrorAndReturnString(context_, exec_status_error, "PJRT_Event_Error failed."));
    }
    destroyEvent();
  }
}

void Event::destroyEvent() {
  PJRT_Event_Destroy_Args event_destroy_args;
  event_destroy_args.struct_size = PJRT_Event_Destroy_Args_STRUCT_SIZE;
  event_destroy_args.extension_start = nullptr;
  event_destroy_args.event = event_;
  context_.pjrtApi_->PJRT_Event_Destroy(&event_destroy_args); // Error on event destroy is usually just logged
  event_ = nullptr;
}
  
} // namespace pjrt