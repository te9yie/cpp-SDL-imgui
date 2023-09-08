#pragma once

#include <cstddef>
#include <cstdint>

namespace t9 {

/// @cond private
namespace detail {

inline std::uint32_t rotate_left_(std::uint32_t x, std::uint32_t n) {
  constexpr std::uint32_t BITS_N = 32;
  return (x << n) | (x >> (BITS_N - n));
}
inline void fxhash_(std::uint32_t* hash, std::uint32_t x) {
  *hash = (rotate_left_(*hash, 5) ^ x) * 0x9e3779b9;
}

}  // namespace detail
/// @endcond

inline void fxhash(std::uint32_t* hash, const void* data, std::size_t n) {
  auto p = reinterpret_cast<const std::uint8_t*>(data);
  std::size_t i = 0;
  while (n - i >= 4) {
    const auto u32 = *reinterpret_cast<const std::uint32_t*>(p + i);
    detail::fxhash_(hash, u32);
    i += 4;
  }
  const auto u32 = *reinterpret_cast<const std::uint32_t*>(p + i);
  // clang-format off
  switch ((n - i) % 4) {
    case 0: break;
    case 1: detail::fxhash_(hash, u32 & 0x000000ff); break;
    case 2: detail::fxhash_(hash, u32 & 0x0000ffff); break;
    case 3: detail::fxhash_(hash, u32 & 0x00ffffff); break;
  }
  // clang-format on
}

inline std::uint32_t fxhash(const void* data, std::size_t n) {
  std::uint32_t x = 0;
  fxhash(&x, data, n);
  return x;
}

}  // namespace t9
