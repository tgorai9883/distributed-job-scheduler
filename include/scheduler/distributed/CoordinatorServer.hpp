#pragma once

#include "scheduler/core/Task.hpp"
#include "scheduler/core/TaskStatus.hpp"
#include "scheduler/distributed/WorkerRegistry.hpp"
#include "scheduler/execution/ExecutionResult.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace scheduler::distributed {

class CoordinatorServer {
public:
    [[nodiscard]] std::string handleMessage(const std::string& textMessage);
    void markDeadWorkers(std::chrono::milliseconds heartbeatTimeout);

    void submitReadyTask(core::Task task);
    [[nodiscard]] std::optional<core::Task> assignTask(const std::string& workerId);
    void reportResult(const std::string& workerId, const execution::ExecutionResult& result);

    void waitForAllTasks();
    [[nodiscard]] core::TaskStatus taskStatus(core::TaskId taskId) const;
    [[nodiscard]] std::size_t pendingTaskCount() const;
    [[nodiscard]] std::size_t runningTaskCount() const;
    [[nodiscard]] std::size_t completedTaskCount() const;
    [[nodiscard]] std::size_t failedTaskCount() const;

    [[nodiscard]] WorkerRegistry& registry();
    [[nodiscard]] const WorkerRegistry& registry() const;

private:
    struct DistributedTaskState {
        core::Task task;
        core::TaskStatus status;
        std::string assignedWorkerId;
    };

    static bool parseResultPayload(
        const std::string& payload,
        core::TaskId& taskId,
        bool& success,
        std::string& errorMessage);

    WorkerRegistry registry_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::unordered_map<core::TaskId, DistributedTaskState> tasks_;
    std::deque<core::TaskId> readyTasks_;
};

} // namespace scheduler::distributed
