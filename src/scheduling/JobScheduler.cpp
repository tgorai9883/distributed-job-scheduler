#include "scheduler/scheduling/JobScheduler.hpp"

#include <stdexcept>
#include <utility>

namespace scheduler::scheduling {

JobScheduler::JobState::JobState(DependencyGraph graph, std::size_t totalTaskCount)
    : graph(std::move(graph)),
      completedTaskCount(0),
      totalTaskCount(totalTaskCount),
      terminal(totalTaskCount == 0)
{
}

JobScheduler::JobScheduler(std::shared_ptr<storage::JobRepository> repository, std::size_t workerCount)
    : repository_(std::move(repository)),
      threadPool_(workerCount),
      shutdown_(false)
{
    if (!repository_) {
        throw std::invalid_argument("job repository must not be null");
    }
}

JobScheduler::~JobScheduler()
{
    shutdown();
}

void JobScheduler::submitJob(core::Job job)
{
    const auto jobId = job.id();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_) {
            throw std::runtime_error("cannot submit job after scheduler shutdown");
        }
    }

    repository_->saveJob(job);

    const auto taskIds = job.getAllTaskIds();
    std::vector<std::pair<core::TaskId, core::TaskId>> dependencies;
    for (const auto& [task, prerequisites] : job.dependencies()) {
        for (const auto prerequisite : prerequisites) {
            dependencies.emplace_back(task, prerequisite);
        }
    }

    DependencyGraph graph(taskIds, dependencies);
    if (graph.hasCycle()) {
        job.setStatus(core::JobStatus::Failed);
        repository_->updateJob(job);
        condition_.notify_all();
        return;
    }

    const auto readyTasks = graph.getInitialReadyTasks();
    job.setStatus(taskIds.empty() ? core::JobStatus::Succeeded : core::JobStatus::Running);
    repository_->updateJob(job);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        jobStates_.insert_or_assign(jobId, JobState(std::move(graph), taskIds.size()));
    }

    if (taskIds.empty()) {
        condition_.notify_all();
        return;
    }

    scheduleTasks(jobId, readyTasks);
}

void JobScheduler::waitForJob(core::JobId jobId)
{
    if (!repository_->exists(jobId)) {
        throw std::runtime_error("job does not exist");
    }

    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [&] {
        const auto job = repository_->getJob(jobId);
        return job.has_value() && isTerminal(job->status());
    });
}

core::JobStatus JobScheduler::getJobStatus(core::JobId jobId)
{
    const auto job = repository_->getJob(jobId);
    if (!job) {
        throw std::runtime_error("job does not exist");
    }

    return job->status();
}

core::TaskStatus JobScheduler::getTaskStatus(core::JobId jobId, core::TaskId taskId)
{
    const auto job = repository_->getJob(jobId);
    if (!job) {
        throw std::runtime_error("job does not exist");
    }

    const auto* task = job->getTask(taskId);
    if (!task) {
        throw std::runtime_error("task does not exist");
    }

    return task->status();
}

void JobScheduler::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_) {
            return;
        }

        shutdown_ = true;
    }

    threadPool_.shutdown();
    condition_.notify_all();
}

bool JobScheduler::isTerminal(core::JobStatus status)
{
    return status == core::JobStatus::Succeeded
        || status == core::JobStatus::Failed
        || status == core::JobStatus::Cancelled;
}

void JobScheduler::scheduleTasks(core::JobId jobId, const std::vector<core::TaskId>& taskIds)
{
    for (const auto taskId : taskIds) {
        scheduleTask(jobId, taskId);
    }
}

void JobScheduler::scheduleTask(core::JobId jobId, core::TaskId taskId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto stateIt = jobStates_.find(jobId);
    if (stateIt == jobStates_.end() || stateIt->second.terminal || shutdown_) {
        return;
    }

    auto job = repository_->getJob(jobId);
    if (!job || isTerminal(job->status())) {
        return;
    }

    auto* task = job->getTask(taskId);
    if (!task) {
        return;
    }

    task->setStatus(core::TaskStatus::Running);
    auto taskToExecute = *task;
    repository_->updateJob(*job);

    try {
        threadPool_.submit([this, jobId, taskId, taskToExecute = std::move(taskToExecute)]() mutable {
            const auto result = taskExecutor_.execute(taskToExecute);
            handleTaskResult(jobId, taskId, result);
        });
    } catch (const std::runtime_error&) {
        job->setStatus(core::JobStatus::Failed);
        stateIt->second.terminal = true;
        repository_->updateJob(*job);
        condition_.notify_all();
    }
}

void JobScheduler::handleTaskResult(
    core::JobId jobId,
    core::TaskId taskId,
    const execution::ExecutionResult& result)
{
    std::vector<core::TaskId> readyTasks;
    bool notify = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto stateIt = jobStates_.find(jobId);
        if (stateIt == jobStates_.end() || stateIt->second.terminal) {
            return;
        }

        auto job = repository_->getJob(jobId);
        if (!job) {
            return;
        }

        auto* task = job->getTask(taskId);
        if (!task) {
            return;
        }

        if (result.success) {
            task->setStatus(core::TaskStatus::Succeeded);
            readyTasks = stateIt->second.graph.markTaskCompleted(taskId);
            ++stateIt->second.completedTaskCount;

            if (stateIt->second.completedTaskCount == stateIt->second.totalTaskCount) {
                job->setStatus(core::JobStatus::Succeeded);
                stateIt->second.terminal = true;
                notify = true;
            }
        } else {
            task->setStatus(core::TaskStatus::Failed);
            job->setStatus(core::JobStatus::Failed);
            stateIt->second.terminal = true;
            notify = true;
        }

        repository_->updateJob(*job);
    }

    if (!readyTasks.empty()) {
        scheduleTasks(jobId, readyTasks);
    }

    if (notify) {
        condition_.notify_all();
    }
}

} // namespace scheduler::scheduling
