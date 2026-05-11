#include "scheduler/distributed/WorkerClient.hpp"

#include "scheduler/distributed/Message.hpp"

#include <utility>

namespace scheduler::distributed {

WorkerClient::WorkerClient(
    std::string workerId,
    CoordinatorServer& coordinator,
    std::chrono::milliseconds heartbeatInterval)
    : workerId_(std::move(workerId)),
      coordinator_(coordinator),
      heartbeatInterval_(heartbeatInterval),
      running_(false)
{
}

WorkerClient::~WorkerClient()
{
    stop();
}

void WorkerClient::registerWithCoordinator()
{
    (void)coordinator_.handleMessage(Message{MessageType::Register, workerId_, ""}.serialize());
}

void WorkerClient::sendHeartbeat()
{
    (void)coordinator_.handleMessage(Message{MessageType::Heartbeat, workerId_, ""}.serialize());
}

bool WorkerClient::executeAssignedTask()
{
    auto task = coordinator_.assignTask(workerId_);
    if (!task) {
        return false;
    }

    const auto result = taskExecutor_.execute(*task);
    coordinator_.reportResult(workerId_, result);
    return true;
}

void WorkerClient::startHeartbeat()
{
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return;
    }

    registerWithCoordinator();
    heartbeatThread_ = std::thread([this] {
        heartbeatLoop();
    });
}

void WorkerClient::stop()
{
    running_ = false;
    if (heartbeatThread_.joinable()) {
        heartbeatThread_.join();
    }
}

void WorkerClient::heartbeatLoop()
{
    while (running_) {
        sendHeartbeat();
        std::this_thread::sleep_for(heartbeatInterval_);
    }
}

} // namespace scheduler::distributed
