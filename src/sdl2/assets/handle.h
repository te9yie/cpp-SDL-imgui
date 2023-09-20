#pragma once

namespace sdl2::assets {

struct AssetId {
  Uint32 index = 0;
  Uint32 revision = 0;

  explicit operator bool() const {
    return revision != 0;
  }

  bool operator==(const AssetId& rhs) const {
    return index == rhs.index && revision == rhs.revision;
  }

  bool operator!=(const AssetId& rhs) const {
    return !operator==(rhs);
  }
};

const AssetId INVALID_ASSET_ID;

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

  AssetHandle(const AssetId& id, AssetHandleObserver* observer) {
    if (id && observer) {
      id_ = id;
      observer_ = observer;
      observer->on_grab(id);
    }
  }

  AssetHandle(const AssetHandle& handle)
      : AssetHandle(handle.id_, handle.observer_) {}

  AssetHandle(AssetHandle&& handle)
      : id_(handle.id_), observer_(handle.observer_) {
    handle.id_ = INVALID_ASSET_ID;
    handle.observer_ = nullptr;
  }

  ~AssetHandle() {
    if (id_ && observer_) {
      observer_->on_drop(id_);
    }
  }

  const AssetId& id() const {
    return id_;
  }

  explicit operator bool() const {
    return id_ && observer_;
  }

  void reset() {
    AssetHandle().swap(*this);
  }

  void reset(const AssetHandle& handle) {
    AssetHandle(handle).swap(*this);
  }

  void reset(AssetHandle&& handle) {
    AssetHandle(std::move(handle)).swap(*this);
  }

  AssetHandle& operator=(const AssetHandle& handle) {
    AssetHandle(handle).swap(*this);
    return *this;
  }

  AssetHandle& operator=(AssetHandle&& handle) {
    AssetHandle(std::move(handle)).swap(*this);
    return *this;
  }

  void swap(AssetHandle& handle) {
    using std::swap;
    swap(id_, handle.id_);
    swap(observer_, handle.observer_);
  }
};

inline void swap(AssetHandle& lhs, AssetHandle& rhs) {
  lhs.swap(rhs);
}

}  // namespace sdl2::assets
