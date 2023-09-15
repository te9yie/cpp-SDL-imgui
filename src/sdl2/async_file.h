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

}  // namespace sdl2
