#include "scheduler/core/Job.hpp"
#include "scheduler/core/JobStatus.hpp"
#include "scheduler/core/Task.hpp"
#include "scheduler/scheduling/JobScheduler.hpp"
#include "scheduler/storage/InMemoryJobRepository.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

std::string toString(scheduler::core::JobStatus status)
{
    switch (status) {
    case scheduler::core::JobStatus::Pending:
        return "Pending";
    case scheduler::core::JobStatus::Running:
        return "Running";
    case scheduler::core::JobStatus::Succeeded:
        return "Succeeded";
    case scheduler::core::JobStatus::Failed:
        return "Failed";
    case scheduler::core::JobStatus::Cancelled:
        return "Cancelled";
    }

    return "Unknown";
}

} // namespace

int main()
{
    auto repository = std::make_shared<scheduler::storage::InMemoryJobRepository>();
    scheduler::scheduling::JobScheduler scheduler(repository, 2);

    scheduler::core::Job job(3, "failure handling");
    job.addTask(scheduler::core::Task(1, "successful setup", [] {
        std::cout << "Task 1 succeeded\n";
    }));
    job.addTask(scheduler::core::Task(2, "failing task", [] {
        std::cout << "Task 2 throwing exception\n";
        throw std::runtime_error("example task failed");
    }));
    job.addTask(scheduler::core::Task(3, "blocked cleanup", [] {
        std::cout << "Task 3 should not run\n";
    }));
    job.addDependency(2, 1);
    job.addDependency(3, 2);

    scheduler.submitJob(job);
    scheduler.waitForJob(job.id());

    std::cout << "Final job status: " << toString(scheduler.getJobStatus(job.id())) << '\n';
    return 0;
}
