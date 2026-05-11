#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace scheduler::distributed {

class WorkerRegistry {
public:
    void registerWorker(const std::string& workerId);
    [[nodiscard]] bool recordHeartbeat(const std::string& workerId);
    std::vector<std::string> markDeadWorkers(std::chrono::milliseconds heartbeatTimeout);

    [[nodiscard]] bool isWorkerAlive(const std::string& workerId) const;
    [[nodiscard]] bool hasWorker(const std::string& workerId) const;
    [[nodiscard]] std::size_t workerCount() const;
    [[nodiscard]] std::vector<std::string> aliveWorkerIds() const;

private:
    struct WorkerInfo {
        std::chrono::steady_clock::time_point lastHeartbeat;
        bool alive;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string, WorkerInfo> workers_;
};

} // namespace scheduler::distributed
