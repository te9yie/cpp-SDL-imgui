#include "loader.h"

namespace sdl2::assets {

/*virtual*/ AssetLoader::~AssetLoader() {
  quit();
}

bool AssetLoader::init() {
  SDL_mutex* mutex = SDL_CreateMutex();
  if (!mutex) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateMutex: %s",
                    SDL_GetError());
    return false;
  }
  remove_mutex_ = mutex;
  return true;
}

void AssetLoader::quit() {
  if (remove_mutex_) {
    SDL_DestroyMutex(remove_mutex_);
    remove_mutex_ = nullptr;
  }
}

AssetHandle AssetLoader::load(std::string_view path) {
  std::string path_str(path);
  if (auto it = id_map_.find(path_str); it != id_map_.end()) {
    return AssetHandle(it->second, this);
  }

  auto index = gen_index_();
  auto asset_data = assets_[index].get();
  AssetId id{index, asset_data->revision};

  id_map_[path_str] = id;
  asset_data->path = path_str;

  return AssetHandle(id, this);
}

void AssetLoader::remove_dropped_assets() {
  std::vector<AssetId> ids;
  {
    SDL_LockMutex(remove_mutex_);
    using std::swap;
    swap(ids, remove_ids_);
    SDL_UnlockMutex(remove_mutex_);
  }
  for (auto& id : ids) {
    auto asset_data = assets_[id.index].get();
    if (SDL_AtomicGet(&asset_data->ref_count) > 0) {
      continue;
    }
    if (auto it = id_map_.find(asset_data->path); it != id_map_.end()) {
      id_map_.erase(it);
    }
    asset_data->path.clear();
    asset_data->revision = std::max<Uint32>(asset_data->revision + 1, 1);
    free_indices_.emplace_back(id.index);
  }
}

Uint32 AssetLoader::gen_index_() {
  if (free_indices_.empty()) {
    auto index = static_cast<Uint32>(assets_.size());
    auto asset_data = std::make_unique<AssetData>();
    asset_data->revision = 1;
    SDL_AtomicSet(&asset_data->ref_count, 0);
    assets_.emplace_back(std::move(asset_data));
    return index;
  } else {
    auto index = free_indices_.front();
    free_indices_.pop_front();
    return index;
  }
}

/*virtual*/ void AssetLoader::on_grab(const AssetId& id) /*override*/ {
  SDL_assert(id);
  auto asset_data = assets_[id.index].get();
  SDL_AtomicIncRef(&asset_data->ref_count);
}

/*virtual*/ void AssetLoader::on_drop(const AssetId& id) /*override*/ {
  SDL_assert(id);
  auto asset_data = assets_[id.index].get();
  if (SDL_AtomicDecRef(&asset_data->ref_count)) {
    SDL_LockMutex(remove_mutex_);
    remove_ids_.emplace_back(id);
    SDL_UnlockMutex(remove_mutex_);
  }
}

void AssetLoader::render_debug_gui() {
  if (ImGui::BeginListBox("Assets")) {
    char buff[1024];
    for (auto& asset_data : assets_) {
      if (SDL_AtomicGet(&asset_data->ref_count) > 0) {
        SDL_snprintf(buff, sizeof(buff), "r#%02d %s",
                     SDL_AtomicGet(&asset_data->ref_count),
                     asset_data->path.c_str());
        ImGui::Selectable(buff);
      }
    }
    ImGui::EndListBox();
  }
}

}  // namespace sdl2::assets
