#include "common.hpp"
#include "context.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif

// Assume pjrt_c_api.h is in the same directory or an include path
#include "pjrt_c_api.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

namespace pjrt {

std::string freeErrorAndReturnString(const Context &context, PJRT_Error *error, const std::string &contextMessage) {
  if (error == nullptr) {
    return {};
  }

  PJRT_Error_Message_Args error_message_args;
  error_message_args.struct_size = PJRT_Error_Message_Args_STRUCT_SIZE;
  error_message_args.extension_start = nullptr;
  error_message_args.error = error;
  context.pjrtApi_->PJRT_Error_Message(&error_message_args);
  const std::string errorMessage = contextMessage + " Error: " + std::string(error_message_args.message, error_message_args.message_size);

  PJRT_Error_Destroy_Args destroy_error_args;
  destroy_error_args.struct_size = PJRT_Error_Destroy_Args_STRUCT_SIZE;
  destroy_error_args.extension_start = nullptr;
  destroy_error_args.error = error;
  context.pjrtApi_->PJRT_Error_Destroy(&destroy_error_args);

  return errorMessage;
}

} // namespace pjrt