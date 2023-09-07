#pragma once

#include "t9/type2int.h"

namespace task {

class Context {
 private:
  std::unordered_map<t9::type_int, void*> pointers_;

 public:
  template <typename T>
  void set(T* p) {
    auto id = t9::type2int<T>::value();
    pointers_[id] = p;
  }

  template <typename T>
  T* get() const {
    auto id = t9::type2int<T>::value();
    auto it = pointers_.find(id);
    if (it == pointers_.end()) {
      return nullptr;
    }
    return reinterpret_cast<T*>(it->second);
  }
};

}  // namespace task
