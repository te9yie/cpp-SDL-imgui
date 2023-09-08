#include "task.h"

namespace task {

/*explicit*/ Task::Task(const Permission& permission)
    : permission_(permission) {}

void Task::exec(const Context& ctx) {
  on_exec(ctx, &work_);
}

}  // namespace task
