#include "scheduler/core/Job.hpp"
#include "scheduler/core/JobStatus.hpp"
#include "scheduler/core/Task.hpp"
#include "scheduler/scheduling/JobScheduler.hpp"
#include "scheduler/storage/InMemoryJobRepository.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

using scheduler::core::Job;
using scheduler::core::JobStatus;
using scheduler::core::Task;
using scheduler::scheduling::JobScheduler;
using scheduler::storage::InMemoryJobRepository;

Job makeIndependentJob(std::size_t taskCount)
{
    Job job(1, "independent");
    for (std::size_t i = 1; i <= taskCount; ++i) {
        job.addTask(Task(i, "task", [] {}));
    }

    return job;
}

Job makeChainJob(std::size_t taskCount)
{
    Job job(1, "chain");
    for (std::size_t i = 1; i <= taskCount; ++i) {
        job.addTask(Task(i, "task", [] {}));
        if (i > 1) {
            job.addDependency(i, i - 1);
        }
    }

    return job;
}

Job makeDiamondBatchJob(std::size_t taskCount)
{
    if (taskCount < 3) {
        throw std::invalid_argument("diamond_batch requires at least 3 tasks");
    }

    Job job(1, "diamond_batch");
    for (std::size_t i = 1; i <= taskCount; ++i) {
        job.addTask(Task(i, "task", [] {}));
    }

    const auto root = 1;
    const auto finalTask = taskCount;
    for (std::size_t i = 2; i < finalTask; ++i) {
        job.addDependency(i, root);
        job.addDependency(finalTask, i);
    }

    return job;
}

Job makeBenchmarkJob(const std::string& scenario, std::size_t taskCount)
{
    if (scenario == "independent") {
        return makeIndependentJob(taskCount);
    }

    if (scenario == "chain") {
        return makeChainJob(taskCount);
    }

    if (scenario == "diamond_batch") {
        return makeDiamondBatchJob(taskCount);
    }

    throw std::invalid_argument("unknown benchmark scenario");
}

int runBenchmark(int argc, char** argv)
{
    if (argc != 5) {
        std::cerr << "Usage: scheduler_app --benchmark <scenario> <task_count> <worker_count>\n";
        return 1;
    }

    const std::string scenario = argv[2];
    const auto taskCount = static_cast<std::size_t>(std::stoull(argv[3]));
    const auto workerCount = static_cast<std::size_t>(std::stoull(argv[4]));

    auto repository = std::make_shared<InMemoryJobRepository>();
    JobScheduler scheduler(repository, workerCount);
    auto job = makeBenchmarkJob(scenario, taskCount);

    const auto startedAt = std::chrono::steady_clock::now();
    scheduler.submitJob(job);
    scheduler.waitForJob(job.id());
    const auto finishedAt = std::chrono::steady_clock::now();

    if (scheduler.getJobStatus(job.id()) != JobStatus::Succeeded) {
        std::cerr << "Benchmark job did not succeed\n";
        return 1;
    }

    const auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(finishedAt - startedAt);
    const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(finishedAt - startedAt).count();
    const auto tasksPerSecond = static_cast<double>(taskCount) / duration.count();

    std::cout << scenario << ','
              << workerCount << ','
              << taskCount << ','
              << totalMs << ','
              << tasksPerSecond << '\n';

    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc > 1 && std::string(argv[1]) == "--benchmark") {
        return runBenchmark(argc, argv);
    }

    std::cout << "Distributed Job Scheduler" << std::endl;
    return 0;
}
