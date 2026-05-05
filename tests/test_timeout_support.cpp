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
#include <thread>

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

void TimedOutTaskFailsJob()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 1);

    Job job(1, "timeout fails");
    job.addTask(Task(1, "slow task", [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }, 0, std::chrono::milliseconds(1)));

    scheduler.submitJob(job);
    scheduler.waitForJob(1);

    const auto savedJob = repository->getJob(1);
    assert(savedJob);
    const auto* task = savedJob->getTask(1);
    assert(task);

    assert(savedJob->status() == JobStatus::Failed);
    assert(task->status() == TaskStatus::Failed);
}

void TimedOutTaskRetriesAndSucceeds()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 1);
    std::atomic<int> attempts = 0;

    Job job(2, "timeout retry");
    job.addTask(Task(1, "eventually fast", [&] {
        if (++attempts == 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    }, 1, std::chrono::milliseconds(5)));

    scheduler.submitJob(job);
    scheduler.waitForJob(2);

    const auto savedJob = repository->getJob(2);
    assert(savedJob);
    const auto* task = savedJob->getTask(1);
    assert(task);

    assert(attempts == 2);
    assert(savedJob->status() == JobStatus::Succeeded);
    assert(task->status() == TaskStatus::Succeeded);
    assert(task->retryCount() == 1);
}

void TimeoutDoesNotUnlockDependentTask()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 2);
    std::atomic<bool> dependentRan = false;

    Job job(3, "timeout dependency");
    job.addTask(Task(1, "slow prerequisite", [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }, 0, std::chrono::milliseconds(1)));
    job.addTask(Task(2, "dependent", [&] {
        dependentRan = true;
    }));
    job.addDependency(2, 1);

    scheduler.submitJob(job);
    scheduler.waitForJob(3);

    const auto savedJob = repository->getJob(3);
    assert(savedJob);
    const auto* prerequisite = savedJob->getTask(1);
    const auto* dependent = savedJob->getTask(2);
    assert(prerequisite);
    assert(dependent);

    assert(savedJob->status() == JobStatus::Failed);
    assert(prerequisite->status() == TaskStatus::Failed);
    assert(dependent->status() == TaskStatus::Pending);
    assert(!dependentRan);
}

} // namespace

int main()
{
    TimedOutTaskFailsJob();
    TimedOutTaskRetriesAndSucceeds();
    TimeoutDoesNotUnlockDependentTask();
    return 0;
}
