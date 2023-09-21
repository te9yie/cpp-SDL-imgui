#pragma once

#include "chunk.h"

namespace sdl2::ecs {

class Archetype final {
 private:
  Uint32 chunk_size_ = 16 * 1024;
  std::vector<std::unique_ptr<Chunk>> chunks_;
  std::vector<Chunk*> archetype_roots_;

 public:
  template <typename... Components>
  Chunk* get_or_new_chunk_for_construct() {
    auto root = find_archetype_root_<Components...>();
    if (root) {
      if (auto chunk = get_not_full_chunk_(root)) {
        return chunk;
      }
    }
    auto chunk =
        std::make_unique<Chunk>(Tuple::make<Components...>(), chunk_size_);
    auto chunk_ptr = chunk.get();
    chunks_.emplace_back(std::move(chunk));
    if (root) {
      link_chunk_(root, chunk_ptr);
    } else {
      archetype_roots_.emplace_back(chunk_ptr);
    }
    return chunk_ptr;
  }

  template <typename... Components>
  Chunk* query_chunk() {
    for (auto& chunk : chunks_) {
      if (chunk->tuple().contains<Components...>()) {
        return chunk;
      }
    }
    return nullptr;
  }

 private:
  template <typename... Components>
  Chunk* find_archetype_root_() {
    for (auto chunk : archetype_roots_) {
      if (chunk->tuple().is_same<Components...>()) {
        return chunk;
      }
    }
    return nullptr;
  }

  Chunk* get_not_full_chunk_(Chunk* root) {
    auto chunk = root;
    while (chunk) {
      if (!chunk->is_full()) {
        return chunk;
      }
      chunk = chunk->next_same_archetype_chunk();
    }
    return nullptr;
  }

  void link_chunk_(Chunk* root, Chunk* chunk) {
    SDL_assert(root);
    auto last_chunk = root;
    while (last_chunk->next_same_archetype_chunk()) {
      last_chunk = last_chunk->next_same_archetype_chunk();
    }
    last_chunk->link_same_archetype_chunk(chunk);
  }
};

}  // namespace sdl2::ecs
