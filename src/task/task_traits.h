#pragma once

#include "context.h"
#include "permission.h"
#include "t9/type_list.h"
#include "work.h"

namespace task {

template <typename T>
struct task_traits {
  static void set_permission(Permission* permission) {
    permission->set_read<T>();
  }
  static T get(const Context& ctx, Work* /*work*/) {
    auto p = ctx.get<T>();
    SDL_assert(p);
    return *p;
  }
};

template <typename T>
struct task_traits<T*> {
  static void set_permission(Permission* permission) {
    permission->set_write<T>();
  }
  static T* get(const Context& ctx, Work* /*work*/) {
    return ctx.get<T>();
  }
};

template <typename T>
struct task_traits<const T*> {
  static void set_permission(Permission* permission) {
    permission->set_read<T>();
  }
  static const T* get(const Context& ctx, Work* /*work*/) {
    return ctx.get<T>();
  }
};

template <typename T>
struct task_traits<T&> {
  static void set_permission(Permission* permission) {
    permission->set_write<T>();
  }
  static T& get(const Context& ctx, Work* /*work*/) {
    auto p = ctx.get<T>();
    SDL_assert(p);
    return *p;
  }
};

template <typename T>
struct task_traits<const T&> {
  static void set_permission(Permission* permission) {
    permission->set_read<T>();
  }
  static const T& get(const Context& ctx, Work* /*work*/) {
    auto p = ctx.get<T>();
    SDL_assert(p);
    return *p;
  }
};

/// @cond private
namespace detail {

inline void set_permission(Permission*, t9::type_list<>) {}

template <typename T, typename... Ts>
inline void set_permission(Permission* permission, t9::type_list<T, Ts...>) {
  task_traits<T>::set_permission(permission);
  set_permission(permission, t9::type_list<Ts...>{});
}

}  // namespace detail
/// @endcond

template <typename... Ts>
inline Permission make_permission() {
  Permission permission;
  detail::set_permission(&permission, t9::type_list<Ts...>{});
  return permission;
}

}  // namespace task
