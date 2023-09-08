#pragma once

#include "task.h"

namespace task {

struct TaskBuilder {
 private:
  Task* task_ = nullptr;

 public:
  TaskBuilder(Task* task) : task_(task) {}
};

}  // namespace task
