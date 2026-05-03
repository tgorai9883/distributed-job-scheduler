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

void TaskThrowsExceptionJobFails()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 1);

    Job job(1, "throws");
    job.addTask(Task(1, "bad task", [] {
        throw std::runtime_error("boom");
    }));

    scheduler.submitJob(job);
    scheduler.waitForJob(1);

    assert(scheduler.getJobStatus(1) == JobStatus::Failed);
    assert(scheduler.getTaskStatus(1, 1) == TaskStatus::Failed);
}

void DependencyCycleJobFails()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 2);
    std::atomic<int> runCount = 0;

    Job job(2, "cycle");
    job.addTask(Task(1, "first", [&] {
        ++runCount;
    }));
    job.addTask(Task(2, "second", [&] {
        ++runCount;
    }));
    job.addDependency(1, 2);
    job.addDependency(2, 1);

    scheduler.submitJob(job);
    scheduler.waitForJob(2);

    assert(scheduler.getJobStatus(2) == JobStatus::Failed);
    assert(runCount == 0);
}

void InvalidDependencyJobFails()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 2);
    std::atomic<int> runCount = 0;

    Job job(3, "invalid dependency");
    job.addTask(Task(1, "only task", [&] {
        ++runCount;
    }));
    job.addDependency(1, 99);

    scheduler.submitJob(job);
    scheduler.waitForJob(3);

    assert(scheduler.getJobStatus(3) == JobStatus::Failed);
    assert(scheduler.getTaskStatus(3, 1) == TaskStatus::Pending);
    assert(runCount == 0);
}

void DuplicateTaskIdRejected()
{
    Job job(4, "duplicate task");
    job.addTask(Task(1, "first", [] {}));

    bool threw = false;
    try {
        job.addTask(Task(1, "duplicate", [] {}));
    } catch (const std::runtime_error&) {
        threw = true;
    }

    assert(threw);
}

void SubmitAfterShutdownThrows()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 1);
    scheduler.shutdown();

    Job job(5, "after shutdown");
    job.addTask(Task(1, "task", [] {}));

    bool threw = false;
    try {
        scheduler.submitJob(job);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    assert(threw);
}

void WaitForUnknownJobThrows()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 1);

    bool threw = false;
    try {
        scheduler.waitForJob(999);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    assert(threw);
}

void GetUnknownTaskStatusThrows()
{
    auto repository = makeRepository();
    JobScheduler scheduler(repository, 1);

    Job job(6, "unknown task");
    job.addTask(Task(1, "task", [] {}));

    scheduler.submitJob(job);
    scheduler.waitForJob(6);

    bool threw = false;
    try {
        (void)scheduler.getTaskStatus(6, 999);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    assert(threw);
}

} // namespace

int main()
{
    TaskThrowsExceptionJobFails();
    DependencyCycleJobFails();
    InvalidDependencyJobFails();
    DuplicateTaskIdRejected();
    SubmitAfterShutdownThrows();
    WaitForUnknownJobThrows();
    GetUnknownTaskStatusThrows();
    return 0;
}
