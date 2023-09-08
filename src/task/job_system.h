#pragma once

#include "job.h"
#include "t9/sdl2/mutex.h"

namespace task {

class JobSystem {
 private:
  JobSystem(const JobSystem&) = delete;
  JobSystem& operator=(const JobSystem&) = delete;

 private:
  std::vector<SDL_Thread*> threads_;
  SDL_atomic_t wait_thread_count_;
  t9::sdl2::MutexPtr mutex_;
  t9::sdl2::ConditionPtr condition_;
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

  void kick_jobs();

  void exec_all_jobs();

 private:
  void exec_jobs_in_thread_();
  void exec_jobs_();
  const char* get_thread_name(SDL_threadID id) const;

 private:
  static int job_thread_(void* args);
};

}  // namespace task
