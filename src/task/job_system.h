#pragma once

#include "job.h"

namespace task {

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

class JobSystem : private JobObserver {
 private:
  JobSystem(const JobSystem&) = delete;
  JobSystem& operator=(const JobSystem&) = delete;

 private:
  std::vector<SDL_Thread*> threads_;
  MutexPtr mutex_;
  ConditionPtr condition_;
  SDL_atomic_t is_quit_;
  SDL_atomic_t job_count_;
  std::list<std::shared_ptr<Job>> jobs_;

 public:
  JobSystem();
  virtual ~JobSystem();

  bool init(std::size_t thread_count);
  void quit();

  bool add_job(std::shared_ptr<Job> job);
  bool insert_job(std::shared_ptr<Job> job);

  void exec_all_jobs();

 private:
  // JobObserver.
  virtual void on_job_done() override;

 private:
  void exec_jobs_in_thread_();
  void exec_jobs_();

 private:
  static int job_thread_(void* args);
};

}  // namespace task
