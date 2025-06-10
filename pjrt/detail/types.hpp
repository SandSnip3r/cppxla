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
#include <type_traits>

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

// Helper to check if PjrtTypeFor<T> is specialized by checking for ::kType
template<typename T, typename = void>
struct is_PjrtTypeFor_specialized : std::false_type {};

template<typename T>
struct is_PjrtTypeFor_specialized<T, std::void_t<decltype(PjrtTypeFor<T>::kType)>> : std::true_type {};

template <typename T>
constexpr PJRT_Buffer_Type TypeToPjrtBufferType() {
  using NonConstT = std::remove_const_t<T>;
  static_assert(is_PjrtTypeFor_specialized<NonConstT>::value, "Unsupported type for PJRT buffer. Please add a specialization to PjrtTypeFor for the non-const version of the type.");
  return PjrtTypeFor<NonConstT>::kType;
}

} // namespace pjrt::detail

#endif // PJRT_TYPES_HPP_