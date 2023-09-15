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
/// @param lhs[in,out]
/// @param rhs[in,out]
inline void swap(Handle& lhs, Handle& rhs) {
  lhs.swap(rhs);
}

/// @brief ハンドル管理されたオブジェクト群
template <typename T>
class HandleObjects : private HandleObserver {
 private:
  HandleObjects(const HandleObjects&) = delete;
  HandleObjects& operator=(const HandleObjects&) = delete;

 private:
  struct Entry {
    SDL_atomic_t ref_count;
    Uint32 revision = 0;
    bool remove_flag = false;
    std::unique_ptr<T> object;
  };

 private:
  Uint32 peak_index_ = 0;
  std::deque<Uint32> index_stock_;
  std::deque<Entry> entries_;
  std::deque<HandleId> remove_ids_;
  MutexPtr remove_mutex_;

 public:
  HandleObjects() = default;

  /// @brief オブジェクトの追加
  /// @param p[in,out] 追加するオブジェクトのポインタ
  /// @return ハンドル
  Handle add(std::unique_ptr<T> p) {
    auto index = find_index_();
    auto& entry = entries_[index];
    entry.object = std::move(p);
    HandleId id{index, entry.revision};
    return Handle(id, this);
  }

  /// @brief オブジェクトの構築
  /// @param args[in] オブジェクトの引数
  /// @return ハンドル
  template <typename... Args>
  Handle emplace(Args&&... args) {
    auto index = find_index_();
    auto& entry = entries_[index];
    entry.object = std::make_unique<T>(std::forward<Args>(args)...);
    HandleId id{index, entry.revision};
    return Handle(id, this);
  }

  /// @brief オブジェクトの削除
  void remove_dropped_objects() {
    std::deque<HandleId> ids;
    {
      ScopedLock lock(remove_mutex_);
      ids.assign(remove_ids_.begin(), remove_ids_.end());
      remove_ids_.clear();
    }
    for (auto& id : ids) {
      auto& entry = entries_[id.index];
      entry.revision = std::max<Uint32>(entry.revision + 1, 1);
      entry.object.reset();
      entry.remove_flag = false;
      index_stock_.emplace_back(id.index);
    }
  }

  /// @brief オブジェクトの取得
  /// @param id[in] ハンドルID
  /// @return オブジェクトのポインタ
  const T* get(const HandleId& id) const {
    if (!exists(id)) {
      return nullptr;
    }
    auto& entry = entries_[id.index];
    return entry.object.get();
  }

  /// @brief オブジェクトの取得
  /// @param id[in] ハンドルID
  /// @return オブジェクトのポインタ
  T* get(const HandleId& id) {
    if (!exists(id)) {
      return nullptr;
    }
    auto& entry = entries_[id.index];
    return entry.object.get();
  }

  /// @brief オブジェクトが存在するか
  /// @param id[in] ハンドルID
  /// @retval true 存在する
  /// @retval false 存在しない
  bool exists(const HandleId& id) const {
    if (!id || id.index >= static_cast<Uint32>(entries_.size())) {
      return false;
    }
    auto& entry = entries_[id.index];
    if (entry.remove_flag) {
      return false;
    }
    return id.revision == entry.revision;
  }

 private:
  std::uint32_t find_index_() {
    if (index_stock_.empty()) {
      SDL_assert(peak_index_ < SDL_MAX_UINT32);
      auto index = peak_index_++;
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
      ScopedLock lock(remove_mutex_);
      remove_ids_.emplace_back(id);
      entry.remove_flag = true;
    }
  }
};

}  // namespace sdl2
