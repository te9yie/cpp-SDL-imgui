#pragma once

namespace sdl2::ecs {

struct Type {
  using Id = std::uintptr_t;
  using CtorFunc = void (*)(void*);
  using DtorFunc = void (*)(void*);

  Id id = 0;
  std::size_t size = 0;
  std::size_t align = 0;
  CtorFunc ctor = nullptr;
  DtorFunc dtor = nullptr;

  template <typename T>
  static std::uintptr_t type_int() {
    static int i;
    return reinterpret_cast<std::uintptr_t>(&i);
  }

  template <typename T>
  static const Type* get() {
    using sanitalized_T =
        std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>;
    return inner_get_<sanitalized_T>();
  }

  template <typename T>
  static const Type* inner_get_() {
    static const Type type = {
        type_int<T>(),
        sizeof(T),
        alignof(T),
        [](void* p) { new (p) T; },
        [](void* p) { std::destroy_at(static_cast<T*>(p)); },
    };
    return &type;
  }
};

template <std::size_t N>
inline void sort(const Type* (&types)[N]) {
  std::sort(types, types + N, [](const Type* lhs, const Type* rhs) {
    if (lhs->size < rhs->size) return false;
    if (lhs->size > rhs->size) return true;
    return lhs->id < rhs->id;
  });
}

}  // namespace sdl2::ecs
