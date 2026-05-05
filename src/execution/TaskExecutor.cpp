#include "scheduler/execution/TaskExecutor.hpp"

#include <chrono>
#include <exception>
#include <string>

namespace scheduler::execution {

ExecutionResult TaskExecutor::execute(core::Task& task)
{
    const auto startedAt = std::chrono::steady_clock::now();

    try {
        task.execute();

        const auto finishedAt = std::chrono::steady_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finishedAt - startedAt);
        if (task.timeout() > std::chrono::milliseconds{0} && duration > task.timeout()) {
            return {
                task.id(),
                false,
                "task timed out",
                duration
            };
        }

        return {
            task.id(),
            true,
            "",
            duration
        };
    } catch (const std::exception& exception) {
        const auto finishedAt = std::chrono::steady_clock::now();
        return {
            task.id(),
            false,
            exception.what(),
            std::chrono::duration_cast<std::chrono::milliseconds>(finishedAt - startedAt)
        };
    } catch (...) {
        const auto finishedAt = std::chrono::steady_clock::now();
        return {
            task.id(),
            false,
            "unknown exception",
            std::chrono::duration_cast<std::chrono::milliseconds>(finishedAt - startedAt)
        };
    }
}

} // namespace scheduler::execution
