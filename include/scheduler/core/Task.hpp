#pragma once

#include "scheduler/core/SchedulerTypes.hpp"
#include "scheduler/core/TaskStatus.hpp"

#include <functional>
#include <string>

namespace scheduler::core {

class Task {
public:
    Task(TaskId id, std::string name, std::function<void()> action, std::uint32_t maxRetries = 0);

    [[nodiscard]] TaskId id() const;
    [[nodiscard]] const std::string& name() const;
    [[nodiscard]] TaskStatus status() const;
    void setStatus(TaskStatus status);

    [[nodiscard]] std::uint32_t retryCount() const;
    [[nodiscard]] std::uint32_t maxRetries() const;
    void incrementRetryCount();

    void execute();

private:
    TaskId id_;
    std::string name_;
    TaskStatus status_;
    std::uint32_t retryCount_;
    std::uint32_t maxRetries_;
    std::function<void()> action_;
};

} // namespace scheduler::core
