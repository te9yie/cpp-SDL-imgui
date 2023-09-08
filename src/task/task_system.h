#pragma once

#include "context.h"
#include "t9/func_traits.h"
#include "t9/type_list.h"
#include "task_job.h"

namespace task {

struct TaskSystemData {
  bool is_loop = true;
};

class TaskSystem {
 private:
  TaskSystem(const TaskSystem&) = delete;
  TaskSystem& operator=(const TaskSystem&) = delete;

 private:
  std::list<std::shared_ptr<TaskJob>> tasks_;
  Context context_;
  TaskSystemData data_;

 public:
  TaskSystem() = default;
  virtual ~TaskSystem() = default;

  template <typename T>
  void set_context(T* p) {
    context_.set(p);
  }

  template <typename F>
  TaskJob* add_task(std::string_view name, F&& f) {
    return add_task_(name, f, t9::args_type_t<F>{});
  }

  void run();

 private:
  template <typename F, typename... As>
  TaskJob* add_task_(std::string_view name, F&& f, t9::type_list<As...>) {
    auto task = std::make_unique<FuncTask<As...>>(f);
    auto task_job = std::make_shared<TaskJob>(name, std::move(task), &context_);
    auto p = task_job.get();
    tasks_.emplace_back(std::move(task_job));
    return p;
  }

  void setup_dependencies_();
};

}  // namespace task
