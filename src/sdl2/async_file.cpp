
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
  data_size_ = static_cast<std::size_t>(size);
  data_ = std::make_unique<uint8_t[]>(data_size_);

  Sint64 loaded_size = 0;
  while (loaded_size < size) {
    auto n = SDL_RWread(rw, data_.get() + loaded_size, 1, (size - loaded_size));
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

Uint8* AsyncFile::release_data() {
  SDL_assert(state_ != State::LOADING);
  state_ = State::NONE;
  data_size_ = 0;
  return data_.release();
}

/*virtual*/ AsyncFileLoader::~AsyncFileLoader() {
  quit();
}

bool AsyncFileLoader::init() {
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "initialize AsyncFileLoader.");

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
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "shutdown AsyncFileLoader.");

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

bool AsyncFileLoader::submit(std::shared_ptr<AsyncFile> file) {
  if (!file) {
    return false;
  }
  if (file->is_state(AsyncFile::State::LOADING)) {
    return false;
  }
  ScopedLock lock(mutex_);
  files_.emplace_back(std::move(file));
  SDL_CondBroadcast(condition_.get());
  return true;
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
