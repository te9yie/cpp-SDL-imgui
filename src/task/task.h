#pragma once

#include "permission.h"
#include "task_traits.h"
#include "work.h"

namespace task {

class Context;

class Task {
 private:
  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

 private:
  Permission permission_;
  Work work_;

 public:
  explicit Task(const Permission& permission);
  virtual ~Task() = default;

  void exec(const Context& ctx);

  const Permission& permission() const {
    return permission_;
  }

 protected:
  virtual void on_exec(const Context& /*ctx*/, Work* /*work*/) = 0;
};

template <typename... As>
class FuncTask : public Task {
 public:
  using Function = std::function<void(As...)>;

 private:
  Function func_;

 public:
  template <typename F>
  FuncTask(F&& f) : Task(make_permission<As...>()), func_(std::forward<F>(f)) {}

 protected:
  virtual void on_exec(const Context& ctx, Work* work) {
    func_(task_traits<As>::get(ctx, work)...);
  }
};

}  // namespace task
