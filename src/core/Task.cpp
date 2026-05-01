#include "scheduler/core/Task.hpp"

#include <utility>

namespace scheduler::core {

Task::Task(TaskId id, std::string name, std::function<void()> action, std::uint32_t maxRetries)
    : id_(id),
      name_(std::move(name)),
      status_(TaskStatus::Pending),
      retryCount_(0),
      maxRetries_(maxRetries),
      action_(std::move(action))
{
}

TaskId Task::id() const
{
    return id_;
}

const std::string& Task::name() const
{
    return name_;
}

TaskStatus Task::status() const
{
    return status_;
}

void Task::setStatus(TaskStatus status)
{
    status_ = status;
}

std::uint32_t Task::retryCount() const
{
    return retryCount_;
}

std::uint32_t Task::maxRetries() const
{
    return maxRetries_;
}

void Task::incrementRetryCount()
{
    ++retryCount_;
}

void Task::execute()
{
    if (action_) {
        action_();
    }
}

} // namespace scheduler::core
