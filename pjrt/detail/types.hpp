#ifndef PJRT_TYPES_HPP_
#define PJRT_TYPES_HPP_

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif

// Assume pjrt_c_api.h is in the same directory or an include path
#include "pjrt_c_api.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <cstdint>

namespace pjrt::detail {

// Helper for type mapping
template <typename T> struct PjrtTypeFor; // Undefined base

// Specializations for common types
template <> struct PjrtTypeFor<float>   { static constexpr PJRT_Buffer_Type kType = PJRT_Buffer_Type_F32; };
template <> struct PjrtTypeFor<double>  { static constexpr PJRT_Buffer_Type kType = PJRT_Buffer_Type_F64; };
template <> struct PjrtTypeFor<int8_t>  { static constexpr PJRT_Buffer_Type kType = PJRT_Buffer_Type_S8;  };
template <> struct PjrtTypeFor<uint8_t> { static constexpr PJRT_Buffer_Type kType = PJRT_Buffer_Type_U8;  };
template <> struct PjrtTypeFor<int16_t> { static constexpr PJRT_Buffer_Type kType = PJRT_Buffer_Type_S16; };
template <> struct PjrtTypeFor<uint16_t>{ static constexpr PJRT_Buffer_Type kType = PJRT_Buffer_Type_U16; };
template <> struct PjrtTypeFor<int32_t> { static constexpr PJRT_Buffer_Type kType = PJRT_Buffer_Type_S32; };
template <> struct PjrtTypeFor<uint32_t>{ static constexpr PJRT_Buffer_Type kType = PJRT_Buffer_Type_U32; };
template <> struct PjrtTypeFor<int64_t> { static constexpr PJRT_Buffer_Type kType = PJRT_Buffer_Type_S64; };
template <> struct PjrtTypeFor<uint64_t>{ static constexpr PJRT_Buffer_Type kType = PJRT_Buffer_Type_U64; };

template <typename T>
constexpr PJRT_Buffer_Type TypeToPjrtBufferType() {
  static_assert(!std::is_same_v<PjrtTypeFor<T>, PjrtTypeFor<void>>, "Unsupported type for PJRT buffer. Please add a specialization to PjrtTypeFor.");
  return PjrtTypeFor<T>::kType;
}
  
} // namespace pjrt::detail

#endif // PJRT_TYPES_HPP_