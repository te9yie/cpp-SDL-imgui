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
  volatile State state_ = State::NONE;
  std::vector<Uint8> data_;

 public:
  explicit AsyncFile(std::string_view path);

  void load();

  const std::string& path() const {
    return path_;
  }

  bool is_loaded() const {
    return state_ == State::LOADED;
  }
  bool is_failed() const {
    return state_ == State::FAILED;
  }

  const std::vector<Uint8>& data() const {
    return data_;
  }
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

  void submit(std::shared_ptr<AsyncFile> file);

 private:
  void load_files_();

 private:
  static int thread_func_(void* args);
};

}  // namespace sdl2
