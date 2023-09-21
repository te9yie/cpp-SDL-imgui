#pragma once

#include "component_traits.h"
#include "tuple.h"

namespace sdl2::ecs {

class Chunk final {
 private:
  Chunk(const Chunk&) = delete;
  Chunk& operator=(const Chunk&) = delete;

 public:
  template <typename... Ts>
  class AccessTuple {
   public:
    using tuple_t = std::tuple<Ts...>;
    template <std::size_t I>
    using element_t = std::tuple_element_t<I, tuple_t>;
    using OffsetArray = std::array<Tuple::Offset, sizeof...(Ts)>;

   private:
    std::uint8_t* base_ = nullptr;
    OffsetArray offsets_;

   public:
    AccessTuple() = default;

    AccessTuple(std::uint8_t* p, const OffsetArray& offsets)
        : base_(p), offsets_(offsets) {}

    template <std::size_t I>
    element_t<I> get() {
      return component_traits<element_t<I>>::get(base_, offsets_[I]);
    }
  };

 public:
  struct Index {
    std::size_t next = 0;
    bool exists = false;
  };

 private:
  Tuple tuple_;
  std::size_t buff_size_ = 0;
  std::unique_ptr<std::uint8_t[]> buff_;
  std::vector<Index> indices_;
  std::size_t peak_index_ = 0;
  std::size_t next_index_ = 0;
  std::size_t exist_count_ = 0;

 public:
  Chunk(Tuple&& tuple, std::size_t buff_size)
      : tuple_(std::move(tuple)),
        buff_size_(std::max<std::size_t>(buff_size, tuple_.memory_size())),
        buff_(std::make_unique<std::uint8_t[]>(buff_size_)),
        indices_(buff_size_ / tuple_.memory_size()) {}

  ~Chunk() {
    SDL_assert(exist_count_ == 0);
  }

  std::size_t construct() {
    assert(next_index_ < capacity());
    if (next_index_ == peak_index_) {
      indices_[peak_index_].next = peak_index_ + 1;
      ++peak_index_;
    }
    auto index = next_index_;
    indices_[index].exists = true;
    next_index_ = indices_[index].next;

    auto p = buff_.get() + index * tuple_.memory_size();
    tuple_.construct(p);
    ++exist_count_;
    return index;
  }

  void destruct(std::size_t index) {
    assert(index < capacity());
    auto p = buff_.get() + index * tuple_.memory_size();
    tuple_.destruct(p);

    indices_[index].exists = false;
    indices_[index].next = next_index_;
    next_index_ = index;
    --exist_count_;
  }

  bool exists(std::size_t index) const {
    assert(index < capacity());
    return indices_[index].exists;
  }

  template <typename... Ts>
  AccessTuple<Ts...> get(std::size_t index) {
    SDL_assert(exists(index));
    return AccessTuple<Ts...>(
        buff_.get() + index * tuple_.memory_size(),
        std::array<Tuple::Offset, sizeof...(Ts)>{tuple_.offset<Ts>()...});
  }

  const Tuple& tuple() const {
    return tuple_;
  }

  std::size_t size() const {
    return peak_index_;
  }

  std::size_t capacity() const {
    return indices_.size();
  }

  bool is_full() const {
    return next_index_ >= capacity();
  }
};

}  // namespace sdl2::ecs

namespace std {

template <typename... Ts>
struct tuple_size<sdl2::ecs::Chunk::AccessTuple<Ts...>>
    : std::integral_constant<size_t, sizeof...(Ts)> {};

template <std::size_t I, typename... Ts>
struct tuple_element<I, sdl2::ecs::Chunk::AccessTuple<Ts...>> {
  using type = std::tuple_element_t<I, std::tuple<Ts...>>;
};

}  // namespace std
