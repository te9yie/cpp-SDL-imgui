
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

/*virtual*/ AsyncFileLoader::~AsyncFileLoader() {
  quit();
}

bool AsyncFileLoader::init() {
  MutexPtr mutex(SDL_CreateMutex());
  if (!mutex) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "SDL_CreateMutex: %s",
                    SDL_GetError());
    return false;
  }

  ConditionPtr condition(SDL_CreateCond());
  if (!condition) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "SDL_CreateCond: %s",
                    SDL_GetError());
    return false;
  }

  mutex_ = std::move(mutex);
  condition_ = std::move(condition);

  thread_ = SDL_CreateThread(thread_func_, "AsyncFileLoader", this);
  if (!thread_) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "SDL_CreateThread: %s",
                    SDL_GetError());
    mutex_.reset();
    condition_.reset();
    return false;
  }

  return true;
}

void AsyncFileLoader::quit() {
  if (thread_) {
    {
      ScopedLock lock(mutex_);
      is_quit_ = true;
      SDL_CondBroadcast(condition_.get());
    }
    SDL_WaitThread(thread_, nullptr);
    thread_ = nullptr;
    is_quit_ = false;
  }
  mutex_.reset();
  condition_.reset();
}

void AsyncFileLoader::submit(std::shared_ptr<AsyncFile> file) {
  ScopedLock lock(mutex_);
  files_.emplace_back(std::move(file));
  SDL_CondBroadcast(condition_.get());
}

void AsyncFileLoader::load_files_() {
  while (!is_quit_) {
    std::shared_ptr<AsyncFile> file;
    {
      ScopedLock lock(mutex_);
      while (files_.empty() && !is_quit_) {
        SDL_CondWait(condition_.get(), mutex_.get());
      }
      if (files_.empty() || is_quit_) {
        continue;
      }
      file = std::move(files_.front());
      files_.pop_front();
    }
    file->load();
  }
}

/*static*/ int AsyncFileLoader::thread_func_(void* args) {
  auto self = static_cast<AsyncFileLoader*>(args);
  self->load_files_();
  return 0;
}

}  // namespace sdl2
