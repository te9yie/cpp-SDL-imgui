#pragma once

#include <cassert>

namespace t9 {

// avoid ADL
namespace singleton_ {

template <typename T>
class Singleton {
 private:
  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;

 private:
  static inline T* instance_ = nullptr;

 protected:
  Singleton() {
    assert(!instance_ && "instance is already.");
    instance_ = static_cast<T*>(this);
  }
  ~Singleton() {
    instance_ = nullptr;
  }

 public:
  static T* instance() {
    return instance_;
  }
};

}  // namespace singleton_

template <typename T>
using Singleton = singleton_::Singleton<T>;

}  // namespace t9
