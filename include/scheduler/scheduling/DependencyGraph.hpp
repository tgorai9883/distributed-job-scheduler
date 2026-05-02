#pragma once

#include "scheduler/core/SchedulerTypes.hpp"

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace scheduler::scheduling {

class DependencyGraph {
public:
    DependencyGraph(
        std::vector<core::TaskId> taskIds,
        const std::vector<std::pair<core::TaskId, core::TaskId>>& dependencies);

    [[nodiscard]] bool hasCycle() const;
    [[nodiscard]] std::vector<core::TaskId> getInitialReadyTasks() const;
    std::vector<core::TaskId> markTaskCompleted(core::TaskId taskId);
    [[nodiscard]] bool isCompleted(core::TaskId taskId) const;

private:
    void addTaskIfMissing(core::TaskId taskId);

    std::vector<core::TaskId> taskIds_;
    std::unordered_map<core::TaskId, std::vector<core::TaskId>> adjacency_;
    std::unordered_map<core::TaskId, std::size_t> indegree_;
    std::unordered_set<core::TaskId> completed_;
};

} // namespace scheduler::scheduling
