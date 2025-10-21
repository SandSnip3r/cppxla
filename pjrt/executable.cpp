#include "context.hpp"
#include "executable.hpp"

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
#include <string>

namespace pjrt {

Executable::Executable(const Context &context, PJRT_Executable *executable) : context_(context), executable_(executable) {
  std::cout << "Constructed Executable" << std::endl;
}

Executable::Executable(Executable &&other) : context_(other.context_), executable_(other.executable_) {
  std::cout << "Move-constructed Executable" << std::endl;
  other.executable_ = nullptr;
}

Executable::~Executable() {
  if (executable_ == nullptr) {
    return;
  }
  PJRT_Executable_Destroy_Args args;
  args.struct_size = PJRT_Executable_Destroy_Args_STRUCT_SIZE;
  args.executable = executable_;
  PJRT_Error *error = context_.pjrtApi_->PJRT_Executable_Destroy(&args);
  if (error != nullptr) {
    const pjrt::Exception ex = context_.convertPjrtErrorToException(error, "PJRT_Executable_Destroy", __FILE__, __LINE__);
    std::cerr << "pjrt::Executable destructor failed to destroy PJRT_Executable: \"" << ex.what() << "\"" << std::endl;
  }
}

void Executable::destroy() {
  if (executable_ == nullptr) {
    return;
  }
  PJRT_Executable_Destroy_Args args;
  args.struct_size = PJRT_Executable_Destroy_Args_STRUCT_SIZE;
  args.executable = executable_;
  PJRT_Error *error = context_.pjrtApi_->PJRT_Executable_Destroy(&args);
  if (error != nullptr) {
    throw context_.convertPjrtErrorToException(error, "PJRT_Executable_Destroy", __FILE__, __LINE__);
  }
}

Executable& Executable::operator=(Executable &&other) {
  executable_ = other.executable_;
  other.executable_ = nullptr;
  return *this;
}

size_t Executable::getNumOutputs() const {
  PJRT_Executable_NumOutputs_Args args;
  args.struct_size = PJRT_Executable_NumOutputs_Args_STRUCT_SIZE;
  args.executable = executable_;
  PJRT_Error *error = context_.pjrtApi_->PJRT_Executable_NumOutputs(&args);
  if (error != nullptr) {
    throw context_.convertPjrtErrorToException(error, "PJRT_Executable_NumOutputs", __FILE__, __LINE__);
  }
  return args.num_outputs;
}

std::vector<std::vector<int64_t>> Executable::getOutputDimensions() const {
  PJRT_Executable_OutputDimensions_Args args;
  args.struct_size = PJRT_Executable_OutputDimensions_Args_STRUCT_SIZE;
  args.executable = executable_;
  PJRT_Error *error = context_.pjrtApi_->PJRT_Executable_OutputDimensions(&args);
  if (error != nullptr) {
    throw context_.convertPjrtErrorToException(error, "PJRT_Executable_OutputDimensions", __FILE__, __LINE__);
  }

  std::vector<std::vector<int64_t>> all_dimensions;
  all_dimensions.reserve(args.num_outputs);
  const int64_t* current_dim_ptr = args.dims;
  for (size_t i = 0; i < args.num_outputs; ++i) {
    size_t num_dims_for_output = args.dim_sizes[i];
    std::vector<int64_t> dimensions(current_dim_ptr, current_dim_ptr + num_dims_for_output);
    all_dimensions.push_back(std::move(dimensions));
    current_dim_ptr += num_dims_for_output;
  }
  return all_dimensions;
}

} // namespace pjrt