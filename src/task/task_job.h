#pragma once

#include "job.h"
#include "task.h"

namespace task {

class Context;

class TaskJob : public Job {
 private:
  std::unique_ptr<Task> task_;
  Context* context_ = nullptr;
  bool check_thread_ = false;
  SDL_threadID thread_id_ = 0;

 public:
  TaskJob(std::string_view name, std::unique_ptr<Task> task, Context* context);

  const Task* task() const {
    return task_.get();
  }

  void set_exec_on_current_thread();

 protected:
  // Job.
  virtual bool on_can_exec() const override;
  virtual void on_exec() override;
};

}  // namespace task
