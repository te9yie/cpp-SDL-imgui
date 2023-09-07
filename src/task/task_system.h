#pragma once

#include "t9/func_traits.h"
#include "t9/type_list.h"
#include "task.h"

namespace task {

class TaskSystem {
 private:
  TaskSystem(const TaskSystem&) = delete;
  TaskSystem& operator=(const TaskSystem&) = delete;

 private:
  std::list<std::shared_ptr<Task>> tasks_;

 public:
  TaskSystem() = default;
  virtual ~TaskSystem() = default;

  template <typename F>
  void add_task(std::string_view name, F&& f) {
    add_task_(name, f, t9::args_type_t<F>{});
  }

  void exec_tasks(const Context& ctx) {
    for (auto& task : tasks_) {
      task->exec(ctx);
    }
  }

 private:
  template <typename F, typename... As>
  void add_task_(std::string_view name, F&& f, t9::type_list<As...>) {
    auto task = std::make_shared<FuncTask<As...>>(name, f);
    tasks_.emplace_back(std::move(task));
  }
};

}  // namespace task
