#pragma once

#include "scheduler/core/SchedulerTypes.hpp"

#include <chrono>
#include <string>

namespace scheduler::execution {

struct ExecutionResult {
    core::TaskId taskId;
    bool success;
    std::string errorMessage;
    std::chrono::milliseconds duration;
};

} // namespace scheduler::execution
