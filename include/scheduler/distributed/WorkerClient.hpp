#pragma once

#include "scheduler/distributed/CoordinatorServer.hpp"
#include "scheduler/execution/TaskExecutor.hpp"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

namespace scheduler::distributed {

class WorkerClient {
public:
    WorkerClient(
        std::string workerId,
        CoordinatorServer& coordinator,
        std::chrono::milliseconds heartbeatInterval = std::chrono::seconds(2));
    ~WorkerClient();

    WorkerClient(const WorkerClient&) = delete;
    WorkerClient& operator=(const WorkerClient&) = delete;

    void registerWithCoordinator();
    void sendHeartbeat();
    [[nodiscard]] bool executeAssignedTask();
    void startHeartbeat();
    void stop();

private:
    void heartbeatLoop();

    std::string workerId_;
    CoordinatorServer& coordinator_;
    execution::TaskExecutor taskExecutor_;
    std::chrono::milliseconds heartbeatInterval_;
    std::atomic<bool> running_;
    std::thread heartbeatThread_;
};

} // namespace scheduler::distributed
