#pragma once

#include "scheduler/concurrency/ThreadPool.hpp"
#include "scheduler/core/Job.hpp"
#include "scheduler/core/JobStatus.hpp"
#include "scheduler/core/SchedulerTypes.hpp"
#include "scheduler/core/TaskStatus.hpp"
#include "scheduler/execution/ExecutionResult.hpp"
#include "scheduler/execution/TaskExecutor.hpp"
#include "scheduler/scheduling/DependencyGraph.hpp"
#include "scheduler/storage/JobRepository.hpp"

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace scheduler::scheduling {

class JobScheduler {
public:
    JobScheduler(std::shared_ptr<storage::JobRepository> repository, std::size_t workerCount);
    ~JobScheduler();

    JobScheduler(const JobScheduler&) = delete;
    JobScheduler& operator=(const JobScheduler&) = delete;

    void submitJob(core::Job job);
    void waitForJob(core::JobId jobId);
    [[nodiscard]] core::JobStatus getJobStatus(core::JobId jobId);
    [[nodiscard]] core::TaskStatus getTaskStatus(core::JobId jobId, core::TaskId taskId);
    void shutdown();

private:
    struct JobState {
        JobState(DependencyGraph graph, std::size_t totalTaskCount);

        DependencyGraph graph;
        std::size_t completedTaskCount;
        std::size_t totalTaskCount;
        bool terminal;
    };

    static bool isTerminal(core::JobStatus status);

    void scheduleTasks(core::JobId jobId, const std::vector<core::TaskId>& taskIds);
    void scheduleTask(core::JobId jobId, core::TaskId taskId);
    void handleTaskResult(core::JobId jobId, core::TaskId taskId, const execution::ExecutionResult& result);

    std::shared_ptr<storage::JobRepository> repository_;
    concurrency::ThreadPool threadPool_;
    execution::TaskExecutor taskExecutor_;
    std::unordered_map<core::JobId, JobState> jobStates_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool shutdown_;
};

} // namespace scheduler::scheduling
