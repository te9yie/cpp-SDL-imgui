#pragma once

#include <cstdint>

namespace t9 {

using type_int = std::uintptr_t;

template <typename T>
struct type2int {
  static type_int value() {
    static int i;
    return reinterpret_cast<type_int>(&i);
  }
};

}  // namespace t9
