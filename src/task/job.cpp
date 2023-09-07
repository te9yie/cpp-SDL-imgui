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

}  // namespace task
