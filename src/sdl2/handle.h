#pragma once

#include "wrap.h"

namespace sdl2 {

/// @brief ハンドルID
struct HandleId {
  Uint32 index = 0;
  Uint32 revision = 0;

  explicit operator bool() const {
    return revision != 0;
  }
};

/// @brief 無効なハンドルID
static inline const HandleId INVALID_HANDLE_ID;

/// @name ハンドルIDの比較
// @{
inline bool operator==(const HandleId& lhs, const HandleId& rhs) {
  return lhs.index == rhs.index && lhs.revision == rhs.revision;
}
inline bool operator!=(const HandleId& lhs, const HandleId& rhs) {
  return !(lhs == rhs);
}
// @}

/// @brief ハンドルのオブザーバー
class HandleObserver {
 public:
  virtual ~HandleObserver() = default;
  virtual void on_grab(const HandleId& id) = 0;
  virtual void on_drop(const HandleId& id) = 0;
};

/// @brief ハンドル
class Handle final {
 private:
  HandleId id_;
  HandleObserver* observer_ = nullptr;

 public:
  Handle() = default;

  Handle(const HandleId& id, HandleObserver* observer)
      : id_(id), observer_(observer) {
    if (id && observer) {
      observer_->on_grab(id);
    }
  }

  Handle(const Handle& handle) : Handle(handle.id_, handle.observer_) {}

  Handle(Handle&& handle) : id_(handle.id_), observer_(handle.observer_) {
    handle.id_ = INVALID_HANDLE_ID;
    handle.observer_ = nullptr;
  }

  ~Handle() {
    if (id_ && observer_) {
      observer_->on_drop(id_);
    }
  }

  const HandleId& id() const {
    return id_;
  }

  explicit operator bool() const {
    return id_ && observer_;
  }

  void clear() {
    Handle().swap(*this);
  }

  Handle& operator=(const Handle& rhs) {
    Handle(rhs).swap(*this);
    return *this;
  }
  Handle& operator=(Handle&& rhs) {
    Handle(std::move(rhs)).swap(*this);
    return *this;
  }

  void swap(Handle& handle) {
    using std::swap;
    swap(id_, handle.id_);
    swap(observer_, handle.observer_);
  }
};

/// @brief swap
inline void swap(Handle& lhs, Handle& rhs) {
  lhs.swap(rhs);
}

/// @brief ハンドル群
class Handles : private HandleObserver {
 private:
  Handles(const Handles&) = delete;
  Handles& operator=(const Handles&) = delete;

 public:
  using OnRemoveFunc = std::function<void(const HandleId&)>;

 private:
  struct Entry {
    mutable SDL_atomic_t ref_count;
    Uint32 revision = 0;
  };

 private:
  std::deque<Entry> entries_;
  std::deque<Uint32> index_stock_;
  std::deque<HandleId> remove_ids_;
  MutexPtr remove_mutex_;

 public:
  Handles() : remove_mutex_(SDL_CreateMutex()) {}

  /// @brief ハンドルの生成
  /// @return ハンドル
  Handle make_handle() {
    auto index = find_index_();
    auto& entry = entries_[index];
    HandleId id{index, entry.revision};
    return Handle(id, this);
  }

  /// @brief 参照されなくなったハンドルの削除
  /// @param on_remove ハンドル削除時に呼び出す関数
  void remove_dropped_handles(const OnRemoveFunc& on_remove = OnRemoveFunc()) {
    std::deque<HandleId> ids;
    {
      ScopedLock lock(remove_mutex_);
      ids.assign(remove_ids_.begin(), remove_ids_.end());
      remove_ids_.clear();
    }
    for (auto& id : ids) {
      index_stock_.emplace_back(id.index);
      if (on_remove) {
        on_remove(id);
      }
    }
  }

  /// @brief ハンドル管理領域のクリア
  void clear() {
    SDL_assert(entries_.size() == index_stock_.size());
    entries_.clear();
    entries_.shrink_to_fit();
    index_stock_.clear();
    index_stock_.shrink_to_fit();
  }

  /// @brief ハンドルが存在するか
  /// @param id[in] ハンドルID
  /// @retval true 存在する
  /// @retval false 存在しない
  bool exists(const HandleId& id) const {
    if (!id || id.index >= static_cast<Uint32>(entries_.size())) {
      return false;
    }
    auto& entry = entries_[id.index];
    if (id.revision != entry.revision) {
      return false;
    }
    return true;
  }

 private:
  std::uint32_t find_index_() {
    if (index_stock_.empty()) {
      auto index = static_cast<Uint32>(entries_.size());
      SDL_assert(index < SDL_MAX_UINT32);
      auto& entry = entries_.emplace_back();
      SDL_AtomicSet(&entry.ref_count, 0);
      entry.revision = 1;
      return index;
    } else {
      auto index = index_stock_.front();
      index_stock_.pop_front();
      return index;
    }
  }

 private:
  // HandleObserver.
  virtual void on_grab(const HandleId& id) override {
    if (!exists(id)) {
      return;
    }
    auto& entry = entries_[id.index];
    SDL_AtomicIncRef(&entry.ref_count);
  }

  virtual void on_drop(const HandleId& id) override {
    if (!exists(id)) {
      return;
    }
    auto& entry = entries_[id.index];
    SDL_assert(SDL_AtomicGet(&entry.ref_count) > 0);
    if (SDL_AtomicDecRef(&entry.ref_count)) {
      entry.revision = std::max<Uint32>(entry.revision + 1, 1);
      ScopedLock lock(remove_mutex_);
      remove_ids_.emplace_back(id);
    }
  }
};

}  // namespace sdl2
