#include "scheduler/concurrency/ThreadPool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>

using scheduler::concurrency::ThreadPool;

namespace {

void ExecutesSingleTask()
{
    ThreadPool pool(1);
    std::atomic<bool> executed = false;

    pool.submit([&] {
        executed = true;
    });

    pool.shutdown();

    assert(executed);
    assert(pool.workerCount() == 1);
}

void ExecutesMultipleTasks()
{
    ThreadPool pool(4);
    std::atomic<int> executedCount = 0;

    for (int i = 0; i < 100; ++i) {
        pool.submit([&] {
            ++executedCount;
        });
    }

    pool.shutdown();

    assert(executedCount == 100);
}

void ExecutesTasksConcurrently()
{
    ThreadPool pool(4);
    constexpr int taskCount = 4;

    std::mutex mutex;
    std::condition_variable condition;
    int readyCount = 0;
    bool releaseTasks = false;

    for (int i = 0; i < taskCount; ++i) {
        pool.submit([&] {
            std::unique_lock<std::mutex> lock(mutex);
            ++readyCount;
            condition.notify_all();
            condition.wait(lock, [&] {
                return releaseTasks;
            });
        });
    }

    bool allTasksReady = false;
    {
        std::unique_lock<std::mutex> lock(mutex);
        allTasksReady = condition.wait_for(lock, std::chrono::seconds(1), [&] {
            return readyCount == taskCount;
        });
        releaseTasks = true;
    }

    condition.notify_all();
    pool.shutdown();

    assert(allTasksReady);
}

void ShutdownIsGraceful()
{
    ThreadPool pool(2);
    std::atomic<int> executedCount = 0;

    for (int i = 0; i < 25; ++i) {
        pool.submit([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            ++executedCount;
        });
    }

    pool.shutdown();

    assert(executedCount == 25);
}

void SubmitAfterShutdownThrows()
{
    ThreadPool pool(1);
    pool.shutdown();

    bool threw = false;
    try {
        pool.submit([] {});
    } catch (const std::runtime_error&) {
        threw = true;
    }

    assert(threw);
}

} // namespace

int main()
{
    ExecutesSingleTask();
    ExecutesMultipleTasks();
    ExecutesTasksConcurrently();
    ShutdownIsGraceful();
    SubmitAfterShutdownThrows();
    return 0;
}
