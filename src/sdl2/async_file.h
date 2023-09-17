#pragma once

#include "wrap.h"

namespace sdl2 {

/// @brief 非同期ファイル
class AsyncFile {
 public:
  enum class State {
    NONE,
    LOADING,
    LOADED,
    FAILED,
  };

 private:
  std::string path_;
  std::size_t data_size_ = 0;
  std::unique_ptr<Uint8[]> data_;
  volatile State state_ = State::NONE;

 public:
  explicit AsyncFile(std::string_view path);

  void load();

  const std::string& path() const {
    return path_;
  }

  bool is_state(State state) const {
    return state_ == state;
  }

  std::size_t data_size() const {
    return data_size_;
  }

  const Uint8* data() const {
    return data_.get();
  }

  Uint8* release_data();
};

/// @brief 非同期ファイル読み込み
class AsyncFileLoader {
 private:
  AsyncFileLoader(const AsyncFileLoader&) = delete;
  AsyncFileLoader& operator=(const AsyncFileLoader&) = delete;

 private:
  std::deque<std::shared_ptr<AsyncFile>> files_;
  SDL_Thread* thread_ = nullptr;
  MutexPtr mutex_;
  ConditionPtr condition_;
  volatile bool is_quit_ = false;

 public:
  AsyncFileLoader() = default;
  virtual ~AsyncFileLoader();

  bool init();
  void quit();

  bool submit(std::shared_ptr<AsyncFile> file);

 private:
  void load_files_();

 private:
  static int thread_func_(void* args);
};

}  // namespace sdl2
