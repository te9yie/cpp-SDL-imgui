#pragma once

namespace task {

template <typename T>
struct WorkRef {
  T* x = nullptr;
  T* operator->() const {
    return x;
  }
  explicit operator bool() const {
    return !!x;
  }
};

template <typename T>
struct task_traits<WorkRef<T>> {
  static void set_permission(Permission*) {}
  static WorkRef<T> get(const Context& /*ctx*/, Work* work) {
    if (!work->exists<T>()) {
      work->emplace<T>();
    }
    return WorkRef<T>{work->get_mut<T>()};
  }
};

}  // namespace task
