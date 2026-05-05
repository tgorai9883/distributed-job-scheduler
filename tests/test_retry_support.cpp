#include "scheduler/core/Job.hpp"
#include "scheduler/core/JobStatus.hpp"
#include "scheduler/core/Task.hpp"
#include "scheduler/core/TaskStatus.hpp"
#include "scheduler/scheduling/JobScheduler.hpp"
#include "scheduler/storage/InMemoryJobRepository.hpp"

#include <atomic>
#include <cassert>
#include <memory>
#include <stdexcept>

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

void FailedTaskRetriesAndSucceeds()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 1);
    std::atomic<int> attempts = 0;

    Job job(1, "retry succeeds");
    job.addTask(Task(1, "flaky task", [&] {
        if (++attempts == 1) {
            throw std::runtime_error("first attempt failed");
        }
    }, 1));

    scheduler.submitJob(job);
    scheduler.waitForJob(1);

    const auto savedJob = repository->getJob(1);
    assert(savedJob);
    const auto* task = savedJob->getTask(1);
    assert(task);

    assert(attempts == 2);
    assert(savedJob->status() == JobStatus::Succeeded);
    assert(task->status() == TaskStatus::Succeeded);
    assert(task->retryCount() == 1);
}

void FailedTaskExhaustsRetriesAndFailsJob()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 1);
    std::atomic<int> attempts = 0;

    Job job(2, "retry exhausts");
    job.addTask(Task(1, "always fails", [&] {
        ++attempts;
        throw std::runtime_error("attempt failed");
    }, 2));

    scheduler.submitJob(job);
    scheduler.waitForJob(2);

    const auto savedJob = repository->getJob(2);
    assert(savedJob);
    const auto* task = savedJob->getTask(1);
    assert(task);

    assert(attempts == 3);
    assert(savedJob->status() == JobStatus::Failed);
    assert(task->status() == TaskStatus::Failed);
    assert(task->retryCount() == 2);
}

void RetryDoesNotUnlockDependentTaskUntilSuccess()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 2);
    std::atomic<int> attempts = 0;
    std::atomic<bool> firstTaskSucceeded = false;
    std::atomic<bool> dependentSawSuccess = false;

    Job job(3, "retry dependency");
    job.addTask(Task(1, "flaky prerequisite", [&] {
        if (++attempts == 1) {
            throw std::runtime_error("first attempt failed");
        }

        firstTaskSucceeded = true;
    }, 1));
    job.addTask(Task(2, "dependent", [&] {
        dependentSawSuccess = firstTaskSucceeded.load();
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

    assert(attempts == 2);
    assert(savedJob->status() == JobStatus::Succeeded);
    assert(prerequisite->status() == TaskStatus::Succeeded);
    assert(prerequisite->retryCount() == 1);
    assert(dependent->status() == TaskStatus::Succeeded);
    assert(dependentSawSuccess);
}

} // namespace

int main()
{
    FailedTaskRetriesAndSucceeds();
    FailedTaskExhaustsRetriesAndFailsJob();
    RetryDoesNotUnlockDependentTaskUntilSuccess();
    return 0;
}
