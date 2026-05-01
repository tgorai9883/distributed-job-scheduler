#pragma once

#include "scheduler/core/JobStatus.hpp"
#include "scheduler/core/SchedulerTypes.hpp"
#include "scheduler/core/Task.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace scheduler::core {

class Job {
public:
    Job(JobId id, std::string name);

    [[nodiscard]] JobId id() const;
    [[nodiscard]] const std::string& name() const;
    [[nodiscard]] JobStatus status() const;
    void setStatus(JobStatus status);

    void addTask(Task task);
    [[nodiscard]] Task* getTask(TaskId taskId);
    [[nodiscard]] const Task* getTask(TaskId taskId) const;
    [[nodiscard]] std::vector<TaskId> getAllTaskIds() const;

    void addDependency(TaskId task, TaskId dependsOn);
    [[nodiscard]] const std::unordered_map<TaskId, std::vector<TaskId>>& dependencies() const;

private:
    JobId id_;
    std::string name_;
    JobStatus status_;
    std::unordered_map<TaskId, Task> tasks_;
    std::unordered_map<TaskId, std::vector<TaskId>> dependencies_;
};

} // namespace scheduler::core
