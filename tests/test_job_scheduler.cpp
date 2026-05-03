#include "scheduler/core/Job.hpp"
#include "scheduler/core/JobStatus.hpp"
#include "scheduler/core/Task.hpp"
#include "scheduler/core/TaskStatus.hpp"
#include "scheduler/scheduling/JobScheduler.hpp"
#include "scheduler/storage/InMemoryJobRepository.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

using scheduler::core::Job;
using scheduler::core::JobStatus;
using scheduler::core::Task;
using scheduler::core::TaskStatus;
using scheduler::scheduling::JobScheduler;
using scheduler::storage::InMemoryJobRepository;

namespace {

std::shared_ptr<InMemoryJobRepository> makeRepository()
{
    return std::make_shared<InMemoryJobRepository>();
}

void IndependentTasksRun()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 2);
    std::atomic<int> runCount = 0;

    Job job(1, "independent");
    job.addTask(Task(1, "first", [&] {
        ++runCount;
    }));
    job.addTask(Task(2, "second", [&] {
        ++runCount;
    }));

    scheduler.submitJob(job);
    scheduler.waitForJob(1);

    assert(runCount == 2);
    assert(scheduler.getJobStatus(1) == JobStatus::Succeeded);
    assert(scheduler.getTaskStatus(1, 1) == TaskStatus::Succeeded);
    assert(scheduler.getTaskStatus(1, 2) == TaskStatus::Succeeded);
}

void DependentTasksRunInOrder()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 2);
    std::vector<int> order;
    std::mutex orderMutex;

    Job job(2, "linear");
    job.addTask(Task(1, "first", [&] {
        std::lock_guard<std::mutex> lock(orderMutex);
        order.push_back(1);
    }));
    job.addTask(Task(2, "second", [&] {
        std::lock_guard<std::mutex> lock(orderMutex);
        order.push_back(2);
    }));
    job.addDependency(2, 1);

    scheduler.submitJob(job);
    scheduler.waitForJob(2);

    assert(scheduler.getJobStatus(2) == JobStatus::Succeeded);
    assert(order.size() == 2);
    assert(order[0] == 1);
    assert(order[1] == 2);
}

void DiamondDependencyRunsCorrectly()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 3);
    std::atomic<bool> firstDone = false;
    std::atomic<int> middleDoneCount = 0;
    std::atomic<bool> finalSawPrerequisites = false;

    Job job(3, "diamond");
    job.addTask(Task(1, "first", [&] {
        firstDone = true;
    }));
    job.addTask(Task(2, "left", [&] {
        assert(firstDone);
        ++middleDoneCount;
    }));
    job.addTask(Task(3, "right", [&] {
        assert(firstDone);
        ++middleDoneCount;
    }));
    job.addTask(Task(4, "final", [&] {
        finalSawPrerequisites = middleDoneCount == 2;
    }));
    job.addDependency(2, 1);
    job.addDependency(3, 1);
    job.addDependency(4, 2);
    job.addDependency(4, 3);

    scheduler.submitJob(job);
    scheduler.waitForJob(3);

    assert(scheduler.getJobStatus(3) == JobStatus::Succeeded);
    assert(finalSawPrerequisites);
    assert(scheduler.getTaskStatus(3, 4) == TaskStatus::Succeeded);
}

void FailedTaskFailsJob()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 2);

    Job job(4, "failure");
    job.addTask(Task(1, "fails", [] {
        throw std::runtime_error("task failed");
    }));
    job.addTask(Task(2, "blocked", [] {}));
    job.addDependency(2, 1);

    scheduler.submitJob(job);
    scheduler.waitForJob(4);

    assert(scheduler.getJobStatus(4) == JobStatus::Failed);
    assert(scheduler.getTaskStatus(4, 1) == TaskStatus::Failed);
    assert(scheduler.getTaskStatus(4, 2) == TaskStatus::Pending);
}

void CycleFailsJob()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 2);
    std::atomic<int> runCount = 0;

    Job job(5, "cycle");
    job.addTask(Task(1, "first", [&] {
        ++runCount;
    }));
    job.addTask(Task(2, "second", [&] {
        ++runCount;
    }));
    job.addDependency(1, 2);
    job.addDependency(2, 1);

    scheduler.submitJob(job);
    scheduler.waitForJob(5);

    assert(scheduler.getJobStatus(5) == JobStatus::Failed);
    assert(runCount == 0);
}

} // namespace

int main()
{
    IndependentTasksRun();
    DependentTasksRunInOrder();
    DiamondDependencyRunsCorrectly();
    FailedTaskFailsJob();
    CycleFailsJob();
    return 0;
}
