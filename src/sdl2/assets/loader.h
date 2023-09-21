#pragma once

#include "../ecs/archetype.h"
#include "../ecs/chunk.h"
#include "handle.h"

namespace sdl2::assets {

class AssetLoader : private AssetHandleObserver {
 private:
  AssetLoader(const AssetLoader&) = delete;
  AssetLoader& operator=(const AssetLoader&) = delete;

 private:
  struct AssetData {
    std::string path;
    Uint32 revision = 0;
    SDL_atomic_t ref_count;
    ecs::Chunk* chunk = nullptr;
    Uint32 chunk_index = 0;
  };

 private:
  std::unordered_map<std::string, AssetId> id_map_;
  std::vector<std::unique_ptr<AssetData>> assets_;
  std::deque<Uint32> free_indices_;
  std::vector<AssetId> remove_ids_;
  SDL_mutex* remove_mutex_ = nullptr;
  ecs::Archetype archetype_;

 public:
  AssetLoader() = default;
  virtual ~AssetLoader();

  bool init();
  void quit();

  AssetHandle load(std::string_view path);

  template <typename... Components>
  ecs::Chunk::AccessTuple<std::add_lvalue_reference_t<Components>...>
  setup_asset(const AssetId& id) {
    SDL_assert(exists(id));
    auto chunk = archetype_.get_or_new_chunk_for_construct<Components...>();
    auto chunk_index = chunk->construct();

    auto asset_data = assets_[id.index].get();
    asset_data->chunk = chunk;
    asset_data->chunk_index = chunk_index;

    return chunk->get<std::add_lvalue_reference_t<Components>...>(chunk_index);
  }

  void remove_dropped_assets();

  template <typename... Components>
  ecs::Chunk::AccessTuple<Components...> get(const AssetId& id) {
    SDL_assert(exists(id));
    auto asset_data = assets_[id.index].get();
    return asset_data->chunk->get<Components...>(asset_data->chunk_index);
  }

  bool exists(const AssetId& id) const;

 private:
  Uint32 gen_index_();

 private:
  // AssetHandleObserver
  virtual void on_grab(const AssetId& id) override;
  virtual void on_drop(const AssetId& id) override;

 public:
  void render_debug_gui();
};

}  // namespace sdl2::assets
