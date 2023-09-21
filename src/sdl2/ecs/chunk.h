#pragma once

#include "component_traits.h"
#include "tuple.h"

namespace sdl2::ecs {

/// @code
/// buff_'s memory layout.
/// +----------------+
/// | Block[0]       |
/// | +------------+ |
/// | | Index[0]   | |
/// | +------------+ |
/// | | Components | |
/// | | ...        | |
/// | +------------+ |
/// +----------------+
/// | Block[1]       |
/// | +------------+ |
/// | | Index[1]   | |
/// | +------------+ |
/// | | Components | |
/// | | ...        | |
/// | +------------+ |
/// ...
/// @endcode
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
    Uint8* base_ = nullptr;
    OffsetArray offsets_;

   public:
    AccessTuple() = default;

    AccessTuple(Uint8* p, const OffsetArray& offsets)
        : base_(p), offsets_(offsets) {}

    template <std::size_t I>
    element_t<I> get() {
      return component_traits<element_t<I>>::get(base_, offsets_[I]);
    }
  };

 public:
  struct Index {
    Uint32 next = 0;
    bool exists = false;
  };

  static_assert(sizeof(Index) % alignof(std::max_align_t) == 0);

  static Uint32 calc_block_size(const Tuple& tuple) {
    auto align = alignof(std::max_align_t);
    auto r = tuple.memory_size() % align;
    auto components_size =
        r == 0 ? tuple.memory_size() : tuple.memory_size() + align - r;
    return static_cast<Uint32>(sizeof(Index) + components_size);
  }

 private:
  Tuple tuple_;
  Uint32 block_size_ = 0;
  Uint32 buff_size_ = 0;
  std::unique_ptr<Uint8[]> buff_;
  Uint32 peak_index_ = 0;
  Uint32 next_index_ = 0;
  Uint32 exist_count_ = 0;
  Chunk* chunk_link_ = nullptr;
  Chunk* same_archetype_chunk_link_ = nullptr;

 public:
  Chunk(Tuple&& tuple, Uint32 buff_size)
      : tuple_(std::move(tuple)),
        block_size_(calc_block_size(tuple_)),
        buff_size_(std::max<Uint32>(buff_size, block_size_)),
        buff_(std::make_unique<Uint8[]>(buff_size_)) {}

  ~Chunk() {
    SDL_assert(exist_count_ == 0);
  }

  Uint32 construct() {
    SDL_assert(next_index_ < capacity());
    if (next_index_ == peak_index_) {
      index_ptr_(peak_index_)->next = peak_index_ + 1;
      ++peak_index_;
    }

    auto index = next_index_;
    index_ptr_(index)->exists = true;
    next_index_ = index_ptr_(index)->next;

    tuple_.construct(components_ptr_(index));
    ++exist_count_;
    return index;
  }

  void destruct(Uint32 index) {
    SDL_assert(index < capacity());
    tuple_.destruct(components_ptr_(index));

    index_ptr_(index)->exists = false;
    index_ptr_(index)->next = next_index_;
    next_index_ = index;
    --exist_count_;
  }

  bool exists(Uint32 index) const {
    SDL_assert(index < capacity());
    return index_ptr_(index)->exists;
  }

  template <typename... Ts>
  AccessTuple<Ts...> get(Uint32 index) {
    SDL_assert(exists(index));
    return AccessTuple<Ts...>(
        components_ptr_(index),
        std::array<Tuple::Offset, sizeof...(Ts)>{tuple_.offset<Ts>()...});
  }

  const Tuple& tuple() const {
    return tuple_;
  }

  Uint32 size() const {
    return peak_index_;
  }

  Uint32 capacity() const {
    return buff_size_ / block_size_;
  }

  bool is_full() const {
    return next_index_ >= capacity();
  }

  void link_chunk(Chunk* c) {
    chunk_link_ = c;
  }
  Chunk* next_chunk() const {
    return chunk_link_;
  }

  void link_same_archetype_chunk(Chunk* c) {
    same_archetype_chunk_link_ = c;
  }
  Chunk* next_same_archetype_chunk() const {
    return same_archetype_chunk_link_;
  }

 private:
  Index* index_ptr_(Uint32 i) {
    return reinterpret_cast<Index*>(buff_.get() + i * block_size_);
  }

  const Index* index_ptr_(Uint32 i) const {
    return reinterpret_cast<const Index*>(buff_.get() + i * block_size_);
  }

  Uint8* components_ptr_(Uint32 i) {
    return buff_.get() + i * block_size_ + sizeof(Index);
  }

  const Uint8* components_ptr_(Uint32 i) const {
    return buff_.get() + i * block_size_ + sizeof(Index);
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
