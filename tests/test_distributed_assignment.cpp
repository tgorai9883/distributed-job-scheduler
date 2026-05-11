#include "scheduler/core/Task.hpp"
#include "scheduler/core/TaskStatus.hpp"
#include "scheduler/distributed/CoordinatorServer.hpp"
#include "scheduler/distributed/WorkerClient.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <thread>

using scheduler::core::Task;
using scheduler::core::TaskStatus;
using scheduler::distributed::CoordinatorServer;
using scheduler::distributed::WorkerClient;

namespace {

void TwoWorkersExecuteAssignedTasks()
{
    CoordinatorServer coordinator;
    WorkerClient workerOne("worker-1", coordinator);
    WorkerClient workerTwo("worker-2", coordinator);
    std::atomic<int> executedCount = 0;

    workerOne.registerWithCoordinator();
    workerTwo.registerWithCoordinator();

    coordinator.submitReadyTask(Task(1, "first", [&] {
        ++executedCount;
    }));
    coordinator.submitReadyTask(Task(2, "second", [&] {
        ++executedCount;
    }));

    assert(workerOne.executeAssignedTask());
    assert(workerTwo.executeAssignedTask());
    coordinator.waitForAllTasks();

    assert(executedCount == 2);
    assert(coordinator.completedTaskCount() == 2);
    assert(coordinator.taskStatus(1) == TaskStatus::Succeeded);
    assert(coordinator.taskStatus(2) == TaskStatus::Succeeded);
}

void FailedTaskResultUpdatesCoordinator()
{
    CoordinatorServer coordinator;
    WorkerClient worker("worker-1", coordinator);
    worker.registerWithCoordinator();

    coordinator.submitReadyTask(Task(1, "fails", [] {
        throw std::runtime_error("failed");
    }));

    assert(worker.executeAssignedTask());
    coordinator.waitForAllTasks();

    assert(coordinator.failedTaskCount() == 1);
    assert(coordinator.taskStatus(1) == TaskStatus::Failed);
}

void DeadWorkerRunningTaskIsRescheduled()
{
    CoordinatorServer coordinator;
    WorkerClient workerOne("worker-1", coordinator);
    WorkerClient workerTwo("worker-2", coordinator);
    std::atomic<int> executedCount = 0;

    workerOne.registerWithCoordinator();
    workerTwo.registerWithCoordinator();

    coordinator.submitReadyTask(Task(1, "rescheduled", [&] {
        ++executedCount;
    }));

    auto assignedTask = coordinator.assignTask("worker-1");
    assert(assignedTask);
    assert(coordinator.runningTaskCount() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    workerTwo.sendHeartbeat();
    coordinator.markDeadWorkers(std::chrono::milliseconds(1));

    assert(!coordinator.registry().isWorkerAlive("worker-1"));
    assert(coordinator.pendingTaskCount() == 1);

    assert(workerTwo.executeAssignedTask());
    coordinator.waitForAllTasks();

    assert(executedCount == 1);
    assert(coordinator.completedTaskCount() == 1);
    assert(coordinator.taskStatus(1) == TaskStatus::Succeeded);
}

} // namespace

int main()
{
    TwoWorkersExecuteAssignedTasks();
    FailedTaskResultUpdatesCoordinator();
    DeadWorkerRunningTaskIsRescheduled();
    return 0;
}
