#include "scheduler/scheduling/DependencyGraph.hpp"

#include <cassert>
#include <unordered_set>
#include <vector>

using scheduler::core::TaskId;
using scheduler::scheduling::DependencyGraph;

namespace {

std::unordered_set<TaskId> toSet(const std::vector<TaskId>& values)
{
    return {values.begin(), values.end()};
}

void NoDependenciesAllReady()
{
    DependencyGraph graph({1, 2, 3}, {});

    assert(!graph.hasCycle());
    assert(toSet(graph.getInitialReadyTasks()) == toSet({1, 2, 3}));
}

void LinearDependency()
{
    DependencyGraph graph({1, 2, 3}, {{2, 1}, {3, 2}});

    assert(!graph.hasCycle());
    assert(toSet(graph.getInitialReadyTasks()) == toSet({1}));

    assert(toSet(graph.markTaskCompleted(1)) == toSet({2}));
    assert(graph.isCompleted(1));
    assert(toSet(graph.markTaskCompleted(2)) == toSet({3}));
    assert(graph.isCompleted(2));
    assert(graph.markTaskCompleted(3).empty());
    assert(graph.isCompleted(3));
}

void DiamondDependency()
{
    DependencyGraph graph({1, 2, 3, 4}, {{2, 1}, {3, 1}, {4, 2}, {4, 3}});

    assert(!graph.hasCycle());
    assert(toSet(graph.getInitialReadyTasks()) == toSet({1}));
    assert(toSet(graph.markTaskCompleted(1)) == toSet({2, 3}));
    assert(graph.markTaskCompleted(2).empty());
    assert(toSet(graph.markTaskCompleted(3)) == toSet({4}));
}

void CycleDetected()
{
    DependencyGraph graph({1, 2, 3}, {{2, 1}, {3, 2}, {1, 3}});

    assert(graph.hasCycle());
    assert(graph.getInitialReadyTasks().empty());
}

void TaskReadyOnlyAfterAllPrerequisitesComplete()
{
    DependencyGraph graph({1, 2, 3}, {{3, 1}, {3, 2}});

    assert(toSet(graph.getInitialReadyTasks()) == toSet({1, 2}));
    assert(graph.markTaskCompleted(1).empty());
    assert(toSet(graph.markTaskCompleted(2)) == toSet({3}));
}

} // namespace

int main()
{
    NoDependenciesAllReady();
    LinearDependency();
    DiamondDependency();
    CycleDetected();
    TaskReadyOnlyAfterAllPrerequisitesComplete();
    return 0;
}
