
#include "async_file.h"

namespace sdl2 {

/*explicit*/ AsyncFile::AsyncFile(std::string_view path) : path_(path) {}

void AsyncFile::load() {
  SDL_assert(state_ == State::NONE);
  state_ = State::LOADING;

  auto rw = SDL_RWFromFile(path_.c_str(), "rb");
  if (!rw) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", path_.c_str(),
                 SDL_GetError());
    state_ = State::FAILED;
    return;
  }

  Sint64 size = SDL_RWsize(rw);
  data_.resize(size);

  Sint64 loaded_size = 0;
  while (loaded_size < size) {
    auto n =
        SDL_RWread(rw, data_.data() + loaded_size, 1, (size - loaded_size));
    loaded_size += n;
    if (n == 0) {
      break;
    }
  }

  SDL_RWclose(rw);

  if (loaded_size != size) {
    state_ = State::FAILED;
    return;
  }

  state_ = State::LOADED;
}

}  // namespace sdl2
