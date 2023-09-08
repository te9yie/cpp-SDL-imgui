#pragma once

#include "t9/type2int.h"

namespace task {

class Work {
 private:
  Work(const Work&) = delete;
  Work& operator=(const Work&) = delete;

 public:
  using AnyMap = std::unordered_map<t9::type_int, std::any>;

 private:
  AnyMap values_;

 public:
  Work() = default;

  template <typename T, typename... Args>
  bool emplace(Args&&... args) {
    auto i = t9::type2int<T>::value();
    auto x = std::make_any<T>(std::forward<Args>(args)...);
    auto r = values_.emplace(i, std::move(x));
    return r.second;
  }

  template <typename T>
  const T* get() const {
    auto i = t9::type2int<T>::value();
    auto it = values_.find(i);
    if (it == values_.end()) return nullptr;
    return &std::any_cast<const T&>(it->second);
  }

  template <typename T>
  T* get_mut() {
    auto i = t9::type2int<T>::value();
    auto it = values_.find(i);
    if (it == values_.end()) return nullptr;
    return &std::any_cast<T&>(it->second);
  }

  template <typename T>
  bool exists() const {
    auto i = t9::type2int<T>::value();
    return values_.find(i) != values_.end();
  }
};

}  // namespace task
