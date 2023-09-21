#pragma once

#include "type.h"

namespace sdl2::ecs {

class Tuple {
 public:
  struct Element {
    const Type* type = nullptr;
    std::size_t offset = 0;
  };

  struct Offset {
    std::size_t offset = 0;
    bool exists = false;
  };

 private:
  std::unique_ptr<Element[]> elements_;
  std::size_t element_count_ = 0;
  std::size_t memory_size_ = 0;

 public:
  template <std::size_t N>
  explicit Tuple(const Type* (&types)[N])
      : elements_(std::make_unique<Element[]>(N)), element_count_(N) {
    const auto fix_align = [](std::size_t offset, std::size_t align) {
      auto r = offset % align;
      return r == 0 ? offset : offset + align - r;
    };
    size_t offset = 0;
    size_t align = 0;
    for (std::size_t i = 0; i < N; ++i) {
      offset = fix_align(offset, types[i]->align);
      elements_[i].type = types[i];
      elements_[i].offset = offset;
      offset += types[i]->size;
      align = std::max(align, types[i]->align);
    }
    offset = fix_align(offset, align);
    memory_size_ = offset;
  }

  Tuple(const Tuple& archetype)
      : elements_(std::make_unique<Element[]>(archetype.element_count_)),
        element_count_(archetype.element_count_),
        memory_size_(archetype.memory_size_) {
    std::copy(archetype.elements_.get(),
              archetype.elements_.get() + archetype.element_count_,
              elements_.get());
  }

  Tuple(Tuple&& archetype)
      : elements_(archetype.elements_.release()),
        element_count_(archetype.element_count_),
        memory_size_(archetype.memory_size_) {
    archetype.element_count_ = 0;
    archetype.memory_size_ = 0;
  }

  const Element& at(std::size_t i) const {
    assert(i < element_count_);
    return elements_[i];
  }

  std::size_t element_count() const {
    return element_count_;
  }

  std::size_t memory_size() const {
    return memory_size_;
  }

  void construct(void* p) const {
    auto top = reinterpret_cast<std::uint8_t*>(p);
    for (std::size_t i = 0; i < element_count_; ++i) {
      elements_[i].type->ctor(top + elements_[i].offset);
    }
  }

  void destruct(void* p) const {
    auto top = reinterpret_cast<std::uint8_t*>(p);
    for (std::size_t i = 0; i < element_count_; ++i) {
      elements_[i].type->dtor(top + elements_[i].offset);
    }
  }

  Tuple& operator=(const Tuple& archetype) {
    Tuple(archetype).swap(*this);
    return *this;
  }

  Tuple& operator=(Tuple&& archetype) {
    Tuple(std::move(archetype)).swap(*this);
    return *this;
  }

  void swap(Tuple& archetype) {
    using std::swap;
    swap(elements_, archetype.elements_);
    swap(element_count_, archetype.element_count_);
    swap(memory_size_, archetype.memory_size_);
  }

  template <typename T>
  bool try_get_offset(std::size_t* out_offset) const {
    assert(out_offset);
    auto type = Type::get<T>();
    for (std::size_t i = 0; i < element_count_; ++i) {
      if (type == elements_[i].type) {
        *out_offset = elements_[i].offset;
        return true;
      }
    }
    return false;
  }

  template <typename T>
  Offset offset() const {
    Offset offset;
    offset.exists = try_get_offset<T>(&offset.offset);
    return offset;
  }

  template <typename... Ts>
  bool is_same() const {
    if (sizeof...(Ts) != element_count_) {
      return false;
    }
    const Type* types[] = {Type::get<Ts>()...};
    sort(types);
    for (std::size_t i = 0; i < element_count_; ++i) {
      if (types[i] != elements_[i].type) {
        return false;
      }
    }
    return true;
  }

  template <typename... Ts>
  bool contains() const {
    if (sizeof...(Ts) > element_count_) {
      return false;
    }
    const Type* types[] = {Type::get<Ts>()...};
    sort(types);
    for (std::size_t i = 0, j = 0; i < element_count_; ++i) {
      if (types[j] == elements_[i].type) {
        if (++j == sizeof...(Ts)) {
          return true;
        }
      }
    }
    return false;
  }

 public:
  template <typename... Ts>
  static Tuple make() {
    const Type* types[] = {Type::get<Ts>()...};
    sort(types);
    return Tuple(types);
  }
};

inline void swap(Tuple& lhs, Tuple& rhs) {
  lhs.swap(rhs);
}

inline bool operator==(const Tuple& lhs, const Tuple& rhs) {
  if (lhs.element_count() != rhs.element_count()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.element_count(); ++i) {
    if (lhs.at(i).type != rhs.at(i).type) {
      return false;
    }
  }
  return true;
}

inline bool operator!=(const Tuple& lhs, const Tuple& rhs) {
  return !(lhs == rhs);
}

}  // namespace sdl2::ecs
