#include "asset_manager.h"

namespace sdl2::ecs {

AssetHandle::AssetHandle(const AssetId& id, AssetHandleObserver* observer)
    : id_(id), observer_(observer) {
  if (id && observer) {
    observer_->on_grab(id);
  }
}

AssetHandle::AssetHandle(const AssetHandle& handle)
    : AssetHandle(handle.id_, handle.observer_) {}

AssetHandle::AssetHandle(AssetHandle&& handle)
    : id_(handle.id_), observer_(handle.observer_) {
  handle.id_ = INVALID_ASSET_ID;
  handle.observer_ = nullptr;
}

AssetHandle::~AssetHandle() {
  if (id_ && observer_) {
    observer_->on_drop(id_);
  }
}

const AssetId& AssetHandle::id() const {
  return id_;
}

/*explicit*/ AssetHandle::operator bool() const {
  return id_ && observer_;
}

void AssetHandle::reset() {
  AssetHandle().swap(*this);
}

void AssetHandle::reset(const AssetHandle& handle) {
  AssetHandle(handle).swap(*this);
}

void AssetHandle::reset(AssetHandle&& handle) {
  AssetHandle(std::move(handle)).swap(*this);
}

AssetHandle& AssetHandle::operator=(const AssetHandle& handle) {
  AssetHandle(handle).swap(*this);
  return *this;
}
AssetHandle& AssetHandle::operator=(AssetHandle&& handle) {
  AssetHandle(std::move(handle)).swap(*this);
  return *this;
}

void AssetHandle::swap(AssetHandle& handle) {
  using std::swap;
  swap(id_, handle.id_);
  swap(observer_, handle.observer_);
}

/*virtual*/ AssetManager::~AssetManager() {
  quit();
}

bool AssetManager::init() {
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "initialize AssetManager.");

  MutexPtr map_mutex(SDL_CreateMutex());
  if (!map_mutex) {
    return false;
  }

  MutexPtr remove_mutex(SDL_CreateMutex());
  if (!remove_mutex) {
    return false;
  }

  if (!loader_.init()) {
    return false;
  }

  map_mutex_ = std::move(map_mutex);
  remove_mutex_ = std::move(remove_mutex);

  return true;
}
void AssetManager::quit() {
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "shutdown AssetManager.");

  loader_.quit();
  map_mutex_.reset();
  remove_mutex_.reset();
}

AssetHandle AssetManager::load(std::string_view path) {
  AssetHandle handle;
  {
    std::string path_str(path);
    ScopedLock lock(map_mutex_);
    if (auto it = asset_map_.find(path_str); it != asset_map_.end()) {
      return AssetHandle(it->second, this);
    }
    auto index = find_index_();
    auto& asset = assets_[index];
    asset.file = std::make_shared<AsyncFile>(path);
    AssetId id{index, asset.revision};
    asset_map_[path_str] = id;
    handle = AssetHandle(id, this);
  }
  auto& asset = assets_[handle.id().index];
  loader_.submit(asset.file);
  return handle;
}

void AssetManager::remove_dropped_assets() {
  std::vector<AssetId> ids;
  {
    ScopedLock lock(remove_mutex_);
    using std::swap;
    swap(ids, remove_ids_);
  }
  for (const auto& id : ids) {
    auto& asset = assets_[id.index];
    {
      ScopedLock lock(map_mutex_);
      if (SDL_AtomicGet(&asset.ref_count) > 0) {
        continue;
      }
      if (auto it = asset_map_.find(asset.file->path());
          it != asset_map_.end()) {
        asset_map_.erase(it);
      }
      asset.revision = std::max<Uint32>(asset.revision + 1, 1);
      asset.file.reset();
      index_stock_.emplace_back(id.index);
    }
  }
}

bool AssetManager::is_loaded(const AssetHandle& handle) const {
  if (!handle) {
    return false;
  }
  const auto& id = handle.id();
  if (id.index >= assets_.size()) {
    return false;
  }
  auto& asset = assets_[id.index];
  if (id.revision != asset.revision) {
    return false;
  }
  return asset.file->is_state(AsyncFile::State::LOADED);
}

bool AssetManager::is_failed(const AssetHandle& handle) const {
  if (!handle) {
    return false;
  }
  const auto& id = handle.id();
  if (id.index >= assets_.size()) {
    return false;
  }
  auto& asset = assets_[id.index];
  if (id.revision != asset.revision) {
    return false;
  }
  return asset.file->is_state(AsyncFile::State::FAILED);
}

Uint32 AssetManager::find_index_() {
  if (index_stock_.empty()) {
    auto index = static_cast<Uint32>(assets_.size());
    SDL_assert(index < SDL_MAX_UINT32);
    auto& entry = assets_.emplace_back();
    SDL_AtomicSet(&entry.ref_count, 0);
    entry.revision = 1;
    return index;
  } else {
    auto index = index_stock_.front();
    index_stock_.pop_front();
    return index;
  }
}

/*virtual*/ void AssetManager::on_grab(const AssetId& id) /*override*/ {
  if (!id) {
    return;
  }
  if (id.index >= assets_.size()) {
    return;
  }
  auto& asset = assets_[id.index];
  if (id.revision != asset.revision) {
    return;
  }
  SDL_AtomicIncRef(&asset.ref_count);
}

/*virtual*/ void AssetManager::on_drop(const AssetId& id) /*override*/ {
  if (!id) {
    return;
  }
  if (id.index >= assets_.size()) {
    return;
  }
  auto& asset = assets_[id.index];
  if (id.revision != asset.revision) {
    return;
  }
  if (SDL_AtomicDecRef(&asset.ref_count)) {
    ScopedLock lock(remove_mutex_);
    remove_ids_.emplace_back(id);
  }
}

}  // namespace sdl2::ecs
