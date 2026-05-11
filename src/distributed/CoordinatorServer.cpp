#include "scheduler/distributed/CoordinatorServer.hpp"

#include "scheduler/distributed/Message.hpp"

#include <sstream>
#include <utility>

namespace scheduler::distributed {

std::string CoordinatorServer::handleMessage(const std::string& textMessage)
{
    const auto message = Message::parse(textMessage);

    if (message.type == MessageType::Register) {
        registry_.registerWorker(message.workerId);
        return Message{MessageType::Ack, "", "registered " + message.workerId}.serialize();
    }

    if (message.type == MessageType::Heartbeat) {
        if (!registry_.recordHeartbeat(message.workerId)) {
            return Message{MessageType::Error, "", "unknown worker " + message.workerId}.serialize();
        }

        return Message{MessageType::Ack, "", "heartbeat " + message.workerId}.serialize();
    }

    if (message.type == MessageType::Result) {
        core::TaskId taskId = 0;
        bool success = false;
        std::string errorMessage;
        if (!parseResultPayload(message.payload, taskId, success, errorMessage)) {
            return Message{MessageType::Error, "", "invalid result"}.serialize();
        }

        reportResult(
            message.workerId,
            execution::ExecutionResult{taskId, success, errorMessage, std::chrono::milliseconds{0}});
        return Message{MessageType::Ack, "", "result " + std::to_string(taskId)}.serialize();
    }

    return Message{MessageType::Error, "", "unknown message"}.serialize();
}

void CoordinatorServer::markDeadWorkers(std::chrono::milliseconds heartbeatTimeout)
{
    const auto deadWorkerIds = registry_.markDeadWorkers(heartbeatTimeout);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& workerId : deadWorkerIds) {
            for (auto& [taskId, taskState] : tasks_) {
                if (taskState.status == core::TaskStatus::Running
                    && taskState.assignedWorkerId == workerId) {
                    taskState.status = core::TaskStatus::Pending;
                    taskState.assignedWorkerId.clear();
                    readyTasks_.push_back(taskId);
                }
            }
        }
    }

    condition_.notify_all();
}

void CoordinatorServer::submitReadyTask(core::Task task)
{
    const auto taskId = task.id();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        task.setStatus(core::TaskStatus::Pending);
        tasks_.insert_or_assign(
            taskId,
            DistributedTaskState{std::move(task), core::TaskStatus::Pending, ""});
        readyTasks_.push_back(taskId);
    }

    condition_.notify_all();
}

std::optional<core::Task> CoordinatorServer::assignTask(const std::string& workerId)
{
    if (!registry_.isWorkerAlive(workerId)) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (readyTasks_.empty()) {
        return std::nullopt;
    }

    for (const auto& [taskId, taskState] : tasks_) {
        (void)taskId;
        if (taskState.status == core::TaskStatus::Running
            && taskState.assignedWorkerId == workerId) {
            return std::nullopt;
        }
    }

    const auto taskId = readyTasks_.front();
    readyTasks_.pop_front();

    auto& taskState = tasks_.at(taskId);
    taskState.status = core::TaskStatus::Running;
    taskState.assignedWorkerId = workerId;
    taskState.task.setStatus(core::TaskStatus::Running);
    return taskState.task;
}

void CoordinatorServer::reportResult(const std::string& workerId, const execution::ExecutionResult& result)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tasks_.find(result.taskId);
        if (it == tasks_.end()) {
            return;
        }

        auto& taskState = it->second;
        if (taskState.status != core::TaskStatus::Running
            || taskState.assignedWorkerId != workerId) {
            return;
        }

        taskState.status = result.success ? core::TaskStatus::Succeeded : core::TaskStatus::Failed;
        taskState.task.setStatus(taskState.status);
        taskState.assignedWorkerId.clear();
    }

    condition_.notify_all();
}

void CoordinatorServer::waitForAllTasks()
{
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] {
        for (const auto& [taskId, taskState] : tasks_) {
            (void)taskId;
            if (taskState.status == core::TaskStatus::Pending
                || taskState.status == core::TaskStatus::Running) {
                return false;
            }
        }

        return true;
    });
}

core::TaskStatus CoordinatorServer::taskStatus(core::TaskId taskId) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    const auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return core::TaskStatus::Cancelled;
    }

    return it->second.status;
}

std::size_t CoordinatorServer::pendingTaskCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t count = 0;
    for (const auto& [taskId, taskState] : tasks_) {
        (void)taskId;
        if (taskState.status == core::TaskStatus::Pending) {
            ++count;
        }
    }

    return count;
}

std::size_t CoordinatorServer::runningTaskCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t count = 0;
    for (const auto& [taskId, taskState] : tasks_) {
        (void)taskId;
        if (taskState.status == core::TaskStatus::Running) {
            ++count;
        }
    }

    return count;
}

std::size_t CoordinatorServer::completedTaskCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t count = 0;
    for (const auto& [taskId, taskState] : tasks_) {
        (void)taskId;
        if (taskState.status == core::TaskStatus::Succeeded) {
            ++count;
        }
    }

    return count;
}

std::size_t CoordinatorServer::failedTaskCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t count = 0;
    for (const auto& [taskId, taskState] : tasks_) {
        (void)taskId;
        if (taskState.status == core::TaskStatus::Failed) {
            ++count;
        }
    }

    return count;
}

WorkerRegistry& CoordinatorServer::registry()
{
    return registry_;
}

const WorkerRegistry& CoordinatorServer::registry() const
{
    return registry_;
}

bool CoordinatorServer::parseResultPayload(
    const std::string& payload,
    core::TaskId& taskId,
    bool& success,
    std::string& errorMessage)
{
    std::istringstream stream(payload);
    std::string status;
    stream >> taskId >> status;

    if (!stream || (status != "SUCCESS" && status != "FAILURE")) {
        return false;
    }

    success = status == "SUCCESS";
    std::getline(stream, errorMessage);
    if (!errorMessage.empty() && errorMessage.front() == ' ') {
        errorMessage.erase(0, 1);
    }

    return true;
}

} // namespace scheduler::distributed
