#include "scheduler/scheduling/DependencyGraph.hpp"

#include <queue>

namespace scheduler::scheduling {

DependencyGraph::DependencyGraph(
    std::vector<core::TaskId> taskIds,
    const std::vector<std::pair<core::TaskId, core::TaskId>>& dependencies)
    : taskIds_(std::move(taskIds))
{
    for (const auto taskId : taskIds_) {
        indegree_.emplace(taskId, 0);
        adjacency_.emplace(taskId, std::vector<core::TaskId>{});
    }

    for (const auto& [task, dependsOn] : dependencies) {
        addTaskIfMissing(task);
        addTaskIfMissing(dependsOn);

        adjacency_[dependsOn].push_back(task);
        ++indegree_[task];
    }
}

bool DependencyGraph::hasCycle() const
{
    auto indegree = indegree_;
    std::queue<core::TaskId> ready;

    for (const auto taskId : taskIds_) {
        if (indegree[taskId] == 0) {
            ready.push(taskId);
        }
    }

    std::size_t visitedCount = 0;
    while (!ready.empty()) {
        const auto taskId = ready.front();
        ready.pop();
        ++visitedCount;

        const auto adjacencyIt = adjacency_.find(taskId);
        if (adjacencyIt == adjacency_.end()) {
            continue;
        }

        for (const auto dependent : adjacencyIt->second) {
            --indegree[dependent];
            if (indegree[dependent] == 0) {
                ready.push(dependent);
            }
        }
    }

    return visitedCount != indegree_.size();
}

std::vector<core::TaskId> DependencyGraph::getInitialReadyTasks() const
{
    std::vector<core::TaskId> readyTasks;

    for (const auto taskId : taskIds_) {
        const auto indegreeIt = indegree_.find(taskId);
        if (indegreeIt != indegree_.end() && indegreeIt->second == 0 && !isCompleted(taskId)) {
            readyTasks.push_back(taskId);
        }
    }

    return readyTasks;
}

std::vector<core::TaskId> DependencyGraph::markTaskCompleted(core::TaskId taskId)
{
    std::vector<core::TaskId> readyTasks;
    if (!completed_.insert(taskId).second) {
        return readyTasks;
    }

    const auto adjacencyIt = adjacency_.find(taskId);
    if (adjacencyIt == adjacency_.end()) {
        return readyTasks;
    }

    for (const auto dependent : adjacencyIt->second) {
        auto indegreeIt = indegree_.find(dependent);
        if (indegreeIt == indegree_.end() || indegreeIt->second == 0) {
            continue;
        }

        --indegreeIt->second;
        if (indegreeIt->second == 0 && !isCompleted(dependent)) {
            readyTasks.push_back(dependent);
        }
    }

    return readyTasks;
}

bool DependencyGraph::isCompleted(core::TaskId taskId) const
{
    return completed_.contains(taskId);
}

void DependencyGraph::addTaskIfMissing(core::TaskId taskId)
{
    if (indegree_.contains(taskId)) {
        return;
    }

    taskIds_.push_back(taskId);
    indegree_.emplace(taskId, 0);
    adjacency_.emplace(taskId, std::vector<core::TaskId>{});
}

} // namespace scheduler::scheduling
