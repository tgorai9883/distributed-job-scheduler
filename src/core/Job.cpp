#include "scheduler/core/Job.hpp"

#include <utility>

namespace scheduler::core {

Job::Job(JobId id, std::string name)
    : id_(id),
      name_(std::move(name)),
      status_(JobStatus::Pending)
{
}

JobId Job::id() const
{
    return id_;
}

const std::string& Job::name() const
{
    return name_;
}

JobStatus Job::status() const
{
    return status_;
}

void Job::setStatus(JobStatus status)
{
    status_ = status;
}

void Job::addTask(Task task)
{
    tasks_.insert_or_assign(task.id(), std::move(task));
}

Task* Job::getTask(TaskId taskId)
{
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return nullptr;
    }

    return &it->second;
}

const Task* Job::getTask(TaskId taskId) const
{
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return nullptr;
    }

    return &it->second;
}

std::vector<TaskId> Job::getAllTaskIds() const
{
    std::vector<TaskId> ids;
    ids.reserve(tasks_.size());

    for (const auto& [taskId, task] : tasks_) {
        (void)task;
        ids.push_back(taskId);
    }

    return ids;
}

void Job::addDependency(TaskId task, TaskId dependsOn)
{
    dependencies_[task].push_back(dependsOn);
}

const std::unordered_map<TaskId, std::vector<TaskId>>& Job::dependencies() const
{
    return dependencies_;
}

} // namespace scheduler::core
