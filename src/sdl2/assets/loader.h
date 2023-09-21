#pragma once

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
  };

 private:
  std::unordered_map<std::string, AssetId> id_map_;
  std::vector<std::unique_ptr<AssetData>> assets_;
  std::deque<Uint32> free_indices_;
  std::vector<AssetId> remove_ids_;
  SDL_mutex* remove_mutex_ = nullptr;

 public:
  AssetLoader() = default;
  virtual ~AssetLoader();

  bool init();
  void quit();

  AssetHandle load(std::string_view path);

  void remove_dropped_assets();

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
