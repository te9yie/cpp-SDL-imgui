#pragma once

#include "../async_file.h"
#include "../wrap.h"

namespace sdl2::ecs {

struct AssetId {
  Uint32 index = 0;
  Uint32 revision = 0;

  explicit operator bool() const {
    return revision != 0;
  }
};

const AssetId INVALID_ASSET_ID;

inline bool operator==(const AssetId& lhs, const AssetId& rhs) {
  return lhs.index == rhs.index && lhs.revision == rhs.revision;
}
inline bool operator!=(const AssetId& lhs, const AssetId& rhs) {
  return !(lhs == rhs);
}

class AssetHandleObserver {
 public:
  virtual ~AssetHandleObserver() = default;
  virtual void on_grab(const AssetId& id) = 0;
  virtual void on_drop(const AssetId& id) = 0;
};

class AssetHandle final {
 private:
  AssetId id_;
  AssetHandleObserver* observer_ = nullptr;

 public:
  AssetHandle() = default;
  AssetHandle(const AssetId& id, AssetHandleObserver* observer);
  AssetHandle(const AssetHandle& handle);
  AssetHandle(AssetHandle&& handle);
  ~AssetHandle();

  const AssetId& id() const;

  explicit operator bool() const;

  void reset();
  void reset(const AssetHandle& handle);
  void reset(AssetHandle&& handle);

  AssetHandle& operator=(const AssetHandle& handle);
  AssetHandle& operator=(AssetHandle&& handle);

  void swap(AssetHandle& handle);
};

class AssetManager : private AssetHandleObserver {
 private:
  AssetManager(const AssetManager&) = delete;
  AssetManager& operator=(const AssetManager&) = delete;

 private:
  struct AssetData {
    SDL_atomic_t ref_count;
    Uint32 revision = 0;
    std::shared_ptr<AsyncFile> file;
  };

 private:
  std::unordered_map<std::string, AssetId> asset_map_;
  MutexPtr map_mutex_;
  std::vector<AssetData> assets_;
  std::deque<Uint32> index_stock_;
  std::vector<AssetId> remove_ids_;
  MutexPtr remove_mutex_;
  AsyncFileLoader loader_;

 public:
  AssetManager() = default;
  virtual ~AssetManager();

  bool init();
  void quit();

  AssetHandle load(std::string_view path);
  void remove_dropped_assets();

  bool is_loaded(const AssetHandle& handle) const;
  bool is_failed(const AssetHandle& handle) const;

 private:
  Uint32 find_index_();

 private:
  // AssetHandleObserver.
  virtual void on_grab(const AssetId& id) override;
  virtual void on_drop(const AssetId& id) override;
};

}  // namespace sdl2::ecs