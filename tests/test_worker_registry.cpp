#include "scheduler/distributed/WorkerRegistry.hpp"

#include <cassert>
#include <chrono>
#include <thread>

using scheduler::distributed::WorkerRegistry;

namespace {

void RegisteredWorkerIsAlive()
{
    WorkerRegistry registry;
    registry.registerWorker("worker-1");

    assert(registry.hasWorker("worker-1"));
    assert(registry.isWorkerAlive("worker-1"));
    assert(registry.workerCount() == 1);
    assert(registry.aliveWorkerIds().size() == 1);
}

void HeartbeatKeepsWorkerAlive()
{
    WorkerRegistry registry;
    registry.registerWorker("worker-1");

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    assert(registry.recordHeartbeat("worker-1"));
    registry.markDeadWorkers(std::chrono::milliseconds(50));

    assert(registry.isWorkerAlive("worker-1"));
}

void MissingHeartbeatMarksWorkerDead()
{
    WorkerRegistry registry;
    registry.registerWorker("worker-1");

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    registry.markDeadWorkers(std::chrono::milliseconds(1));

    assert(registry.hasWorker("worker-1"));
    assert(!registry.isWorkerAlive("worker-1"));
    assert(registry.aliveWorkerIds().empty());
}

void UnknownHeartbeatIsRejected()
{
    WorkerRegistry registry;
    assert(!registry.recordHeartbeat("missing-worker"));
    assert(!registry.hasWorker("missing-worker"));
}

} // namespace

int main()
{
    RegisteredWorkerIsAlive();
    HeartbeatKeepsWorkerAlive();
    MissingHeartbeatMarksWorkerDead();
    UnknownHeartbeatIsRejected();
    return 0;
}
