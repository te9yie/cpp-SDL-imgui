#include "task_system.h"

#include "debug/perf.h"
#include "job_system.h"
#include "permission.h"

namespace task {

void TaskSystem::run() {
  context_.set(&data_);
  setup_dependencies_();
  auto jobs = context_.get<JobSystem>();
  SDL_assert(jobs);
  PERF_SETUP("Main Thread");
  while (data_.is_loop) {
    PERF_SWAP();
    {
      PERF_TAG("setup jobs");
      for (auto& task_job : tasks_) {
        task_job->reset();
      }
      for (auto& task_job : tasks_) {
        jobs->add_job(task_job);
      }
    }
    jobs->exec_all_jobs();
  }
}

void TaskSystem::setup_dependencies_() {
  for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
    using rev_iter = std::reverse_iterator<decltype(it)>;
    TaskJob* task_job = it->get();
    const Permission& permission = task_job->task()->permission();

    for (auto i : permission.writes) {
      for (auto r_it = rev_iter(it), r_end = tasks_.rend(); r_it != r_end;
           ++r_it) {
        if ((*r_it)->task()->permission().is_conflict_write(i)) {
          task_job->add_prerequisite(*r_it);
        }
      }
    }

    for (auto i : permission.reads) {
      for (auto r_it = rev_iter(it), r_end = tasks_.rend(); r_it != r_end;
           ++r_it) {
        if ((*r_it)->task()->permission().is_conflict_read(i)) {
          task_job->add_prerequisite(*r_it);
        }
      }
    }
  }
}

}  // namespace task
