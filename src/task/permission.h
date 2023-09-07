#pragma once

#include "t9/type2int.h"

namespace task {

struct Permission {
  std::set<t9::type_int> writes;
  std::set<t9::type_int> reads;

  template <typename T>
  void set_write() {
    writes.emplace(t9::type2int<T>::value());
  }

  template <typename T>
  void set_read() {
    reads.emplace(t9::type2int<T>::value());
  }

  bool is_conflict_write(t9::type_int i) const {
    if (writes.find(i) != writes.end()) return true;
    if (reads.find(i) != reads.end()) return true;
    return false;
  }

  bool is_conflict_read(t9::type_int i) const {
    if (writes.find(i) != writes.end()) return true;
    return false;
  }
};

}  // namespace task
