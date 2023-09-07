#include "task.h"

namespace task {

Task::Task(std::string_view name, const Permission& permission)
    : name_(name), permission_(permission) {}

void Task::exec(const Context& ctx) {
  on_exec(ctx, &work_);
}

}  // namespace task
