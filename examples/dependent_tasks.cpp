#include "scheduler/core/Job.hpp"
#include "scheduler/core/Task.hpp"
#include "scheduler/scheduling/JobScheduler.hpp"
#include "scheduler/storage/InMemoryJobRepository.hpp"

#include <iostream>
#include <memory>
#include <mutex>

int main()
{
    auto repository = std::make_shared<scheduler::storage::InMemoryJobRepository>();
    scheduler::scheduling::JobScheduler scheduler(repository, 2);
    std::mutex logMutex;

    auto log = [&logMutex](const char* message) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << message << '\n';
    };

    scheduler::core::Job job(2, "dependent tasks");
    job.addTask(scheduler::core::Task(1, "task 1", [&] {
        log("Task 1 complete");
    }));
    job.addTask(scheduler::core::Task(2, "task 2", [&] {
        log("Task 2 complete after task 1");
    }));
    job.addTask(scheduler::core::Task(3, "task 3", [&] {
        log("Task 3 complete after task 1");
    }));
    job.addTask(scheduler::core::Task(4, "task 4", [&] {
        log("Task 4 complete after tasks 2 and 3");
    }));

    job.addDependency(2, 1);
    job.addDependency(3, 1);
    job.addDependency(4, 2);
    job.addDependency(4, 3);

    scheduler.submitJob(job);
    scheduler.waitForJob(job.id());

    std::cout << "Dependent job finished\n";
    return 0;
}
