#pragma once

#include <SDL.h>

#include <memory>

namespace t9::sdl2 {

struct DestroyMutex {
  void operator()(SDL_mutex* m) const {
    SDL_DestroyMutex(m);
  }
};
using MutexPtr = std::unique_ptr<SDL_mutex, DestroyMutex>;

struct DestroyCondition {
  void operator()(SDL_cond* c) const {
    SDL_DestroyCond(c);
  }
};
using ConditionPtr = std::unique_ptr<SDL_cond, DestroyCondition>;

class ScopedLock {
 private:
  SDL_mutex* mutex_ = nullptr;

 public:
  explicit ScopedLock(SDL_mutex* mutex) : mutex_(mutex) {
    SDL_LockMutex(mutex_);
  }
  ~ScopedLock() {
    SDL_UnlockMutex(mutex_);
  }
};

}  // namespace t9::sdl2
