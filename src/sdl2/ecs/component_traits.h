#pragma once

#include "tuple.h"

namespace sdl2::ecs {

template <typename T>
struct component_traits {
  static T get(std::uint8_t* p, const Tuple::Offset& offset) {
    assert(offset.exists);
    return *reinterpret_cast<T*>(p + offset.offset);
  }
};

template <typename T>
struct component_traits<T*> {
  static T* get(std::uint8_t* p, const Tuple::Offset& offset) {
    return offset.exists ? reinterpret_cast<T*>(p + offset.offset) : nullptr;
  }
};

template <typename T>
struct component_traits<const T*> {
  static const T* get(std::uint8_t* p, const Tuple::Offset& offset) {
    return offset.exists ? reinterpret_cast<T*>(p + offset.offset) : nullptr;
  }
};

template <typename T>
struct component_traits<T&> {
  static T& get(std::uint8_t* p, const Tuple::Offset& offset) {
    assert(offset.exists);
    return *reinterpret_cast<T*>(p + offset.offset);
  }
};

template <typename T>
struct component_traits<const T&> {
  static const T& get(std::uint8_t* p, const Tuple::Offset& offset) {
    assert(offset.exists);
    return *reinterpret_cast<T*>(p + offset.offset);
  }
};

}  // namespace sdl2::ecs
