#pragma once

#include "scheduler/core/Task.hpp"
#include "scheduler/execution/ExecutionResult.hpp"

namespace scheduler::execution {

class TaskExecutor {
public:
    [[nodiscard]] ExecutionResult execute(core::Task& task);
};

} // namespace scheduler::execution
