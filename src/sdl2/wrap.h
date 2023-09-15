#pragma once

namespace sdl2 {

/// @cond private
namespace detail {

struct DestroyWindow {
  void operator()(SDL_Window* w) const {
    SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "destroy window.");
    SDL_DestroyWindow(w);
  }
};

struct DestroyRenderer {
  void operator()(SDL_Renderer* r) const {
    SDL_LogDebug(SDL_LOG_CATEGORY_RENDER, "destroy renderer.");
    SDL_DestroyRenderer(r);
  }
};

struct DestroyMutex {
  void operator()(SDL_mutex* m) const {
    SDL_DestroyMutex(m);
  }
};

struct DestroyCondition {
  void operator()(SDL_cond* c) const {
    SDL_DestroyCond(c);
  }
};

}  // namespace detail
/// @endcond

using WindowPtr = std::unique_ptr<SDL_Window, detail::DestroyWindow>;
using RendererPtr = std::unique_ptr<SDL_Renderer, detail::DestroyRenderer>;
using MutexPtr = std::unique_ptr<SDL_mutex, detail::DestroyMutex>;
using ConditionPtr = std::unique_ptr<SDL_cond, detail::DestroyCondition>;

class ScopedLock final {
 private:
  ScopedLock(const ScopedLock&) = delete;
  ScopedLock& operator=(const ScopedLock&) = delete;

 private:
  SDL_mutex* mutex_ = nullptr;

 public:
  ScopedLock(const MutexPtr& m) : mutex_(m.get()) {
    SDL_assert(mutex_);
    SDL_LockMutex(mutex_);
  }
  ~ScopedLock() {
    SDL_UnlockMutex(mutex_);
  }
};

}  // namespace sdl2
