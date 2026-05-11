#include "scheduler/distributed/WorkerRegistry.hpp"

namespace scheduler::distributed {

void WorkerRegistry::registerWorker(const std::string& workerId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    workers_[workerId] = {std::chrono::steady_clock::now(), true};
}

bool WorkerRegistry::recordHeartbeat(const std::string& workerId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = workers_.find(workerId);
    if (it == workers_.end()) {
        return false;
    }

    it->second.lastHeartbeat = std::chrono::steady_clock::now();
    it->second.alive = true;
    return true;
}

std::vector<std::string> WorkerRegistry::markDeadWorkers(std::chrono::milliseconds heartbeatTimeout)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = std::chrono::steady_clock::now();
    std::vector<std::string> deadWorkerIds;

    for (auto& [workerId, worker] : workers_) {
        if (worker.alive && now - worker.lastHeartbeat > heartbeatTimeout) {
            worker.alive = false;
            deadWorkerIds.push_back(workerId);
        }
    }

    return deadWorkerIds;
}

bool WorkerRegistry::isWorkerAlive(const std::string& workerId) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    const auto it = workers_.find(workerId);
    return it != workers_.end() && it->second.alive;
}

bool WorkerRegistry::hasWorker(const std::string& workerId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return workers_.contains(workerId);
}

std::size_t WorkerRegistry::workerCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return workers_.size();
}

std::vector<std::string> WorkerRegistry::aliveWorkerIds() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> workerIds;

    for (const auto& [workerId, worker] : workers_) {
        if (worker.alive) {
            workerIds.push_back(workerId);
        }
    }

    return workerIds;
}

} // namespace scheduler::distributed
