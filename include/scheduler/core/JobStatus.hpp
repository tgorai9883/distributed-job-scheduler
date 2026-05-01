#pragma once

namespace scheduler::core {

enum class JobStatus {
    Pending,
    Running,
    Succeeded,
    Failed,
    Cancelled
};

} // namespace scheduler::core
