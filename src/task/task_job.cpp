#include "task_job.h"

namespace task {

TaskJob::TaskJob(std::string_view name, std::unique_ptr<Task> task,
                 Context* context)
    : Job(name), task_(std::move(task)), context_(context) {}

void TaskJob::set_exec_on_current_thread() {
  check_thread_ = true;
  thread_id_ = SDL_ThreadID();
}

/*virtual*/ bool TaskJob::on_can_exec() const /*override*/ {
  if (check_thread_) {
    if (thread_id_ != SDL_ThreadID()) {
      return false;
    }
  }
  return true;
}

/*virtual*/ void TaskJob::on_exec() /*override*/ {
  task_->exec(*context_);
}

}  // namespace task
