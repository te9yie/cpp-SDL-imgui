#include "job.h"

namespace task {

bool Job::can_submit() const {
  return state_ == State::NONE;
}
void Job::submit(JobObserver* observer) {
  observer_ = observer;
  SDL_AtomicSet(&prerequisite_count_, static_cast<int>(prerequisites_.size()));
  SDL_AtomicSet(&child_count_, 0);
  state_ = State::WAIT_EXEC;
}

bool Job::can_exec() const {
  if (state_ != State::WAIT_EXEC) {
    return false;
  }
  if (SDL_AtomicGet(const_cast<SDL_atomic_t*>(&prerequisite_count_)) > 0) {
    return false;
  }
  return on_can_exec();
}

void Job::exec() {
  state_ = State::EXEC;
  on_exec();
  state_ = State::WAIT_DONE;
}

bool Job::can_done() const {
  if (state_ != State::WAIT_DONE) {
    return false;
  }
  if (SDL_AtomicGet(const_cast<SDL_atomic_t*>(&child_count_)) > 0) {
    return false;
  }
  return true;
}

void Job::done() {
  state_ = State::DONE;
  for (Job* dependency : dependencies_) {
    dependency->dec_prerequisite_count_();
  }
  if (parent_) {
    parent_->dec_child_count_();
  }
  if (observer_) {
    observer_->on_job_done();
  }
}

bool Job::add_prerequisite(const std::shared_ptr<Job>& job) {
  if (state_ != State::NONE) {
    return false;
  }
  if (job->state_ != State::NONE) {
    return false;
  }
  job->dependencies_.emplace_back(this);
  prerequisites_.emplace_back(job);
  return true;
}

bool Job::add_child(const std::shared_ptr<Job>& job) {
  if (state_ != State::EXEC) {
    return false;
  }
  if (job->state_ != State::NONE) {
    return false;
  }
  job->parent_ = this;
  SDL_AtomicIncRef(&child_count_);
  return true;
}

void Job::dec_prerequisite_count_() {
  SDL_assert(SDL_AtomicGet(&prerequisite_count_) > 0);
  SDL_AtomicDecRef(&prerequisite_count_);
}

void Job::dec_child_count_() {
  SDL_assert(SDL_AtomicGet(&child_count_) > 0);
  SDL_AtomicDecRef(&child_count_);
}

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
