#include "job_system.h"

namespace task {

JobSystem::JobSystem() {}

/*virtual*/ JobSystem::~JobSystem() {
  quit();
}

bool JobSystem::init(std::size_t thread_count) {
  SDL_AtomicSet(&is_quit_, 0);

  // mutex.
  MutexPtr mutex(SDL_CreateMutex());
  if (!mutex) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "SDL_CreateMutex: %s",
                    SDL_GetError());
    return false;
  }

  // condition.
  ConditionPtr condition(SDL_CreateCond());
  if (!condition) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "SDL_CreateCond: %s",
                    SDL_GetError());
    return false;
  }

  mutex_ = std::move(mutex);
  condition_ = std::move(condition);

  // thread.
  bool is_success = true;
  threads_.resize(thread_count);
  char name[128];
  for (std::size_t i = 0; i < thread_count && is_success; ++i) {
    SDL_snprintf(name, sizeof(name), "JobThread %2u", i);
    auto thread = threads_[i] =
        SDL_CreateThread(JobSystem::job_thread_, name, this);
    if (!thread) {
      SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "SDL_CreateThread(%s): %s", name,
                      SDL_GetError());
      is_success = false;
    }
  }

  if (!is_success) {
    quit();
    return false;
  }

  return true;
}

void JobSystem::quit() {
  if (!mutex_) return;

  SDL_LockMutex(mutex_.get());
  SDL_AtomicSet(&is_quit_, 1);
  SDL_CondBroadcast(condition_.get());
  SDL_UnlockMutex(mutex_.get());

  for (SDL_Thread* thread : threads_) {
    if (!thread) continue;
    SDL_WaitThread(thread, nullptr);
  }
  threads_.clear();

  mutex_.reset();
  condition_.reset();
}

bool JobSystem::add_job(std::shared_ptr<Job> job) {
  if (!job || !job->can_submit()) {
    return false;
  }

  SDL_LockMutex(mutex_.get());
  job->submit(this);
  jobs_.emplace_back(std::move(job));
  SDL_CondSignal(condition_.get());
  SDL_UnlockMutex(mutex_.get());

  return true;
}

bool JobSystem::insert_job(std::shared_ptr<Job> job) {
  if (!job || !job->can_submit()) {
    return false;
  }

  SDL_LockMutex(mutex_.get());
  job->submit(this);
  jobs_.emplace_front(std::move(job));
  SDL_CondSignal(condition_.get());
  SDL_UnlockMutex(mutex_.get());

  return true;
}

/*virtual*/ void JobSystem::on_job_done() /*override*/ {
  SDL_LockMutex(mutex_.get());
  SDL_CondSignal(condition_.get());
  SDL_UnlockMutex(mutex_.get());
}

void JobSystem::exec_jobs_() {
  while (SDL_AtomicGet(&is_quit_) == 0) {
    std::shared_ptr<Job> job;
    {
      SDL_LockMutex(mutex_.get());
      for (auto it = jobs_.begin(); it != jobs_.end(); ++it) {
        if ((*it)->can_exec() || (*it)->can_done()) {
          job = std::move(*it);
          jobs_.erase(it);
          break;
        }
      }
      if (!job) {
        SDL_CondWait(condition_.get(), mutex_.get());
      }
      SDL_UnlockMutex(mutex_.get());
    }
    if (!job) {
      continue;
    }
    if (job->can_exec()) {
      job->exec();
    }
    if (job->can_done()) {
      job->done();
    } else {
      SDL_LockMutex(mutex_.get());
      jobs_.emplace_front(std::move(job));
      SDL_UnlockMutex(mutex_.get());
    }
  }
}

/*static*/ int JobSystem::job_thread_(void* args) {
  auto self = static_cast<JobSystem*>(args);
  self->exec_jobs_();
  return 0;
}

}  // namespace task
