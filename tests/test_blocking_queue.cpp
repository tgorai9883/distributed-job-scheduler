#include "scheduler/concurrency/BlockingQueue.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

using scheduler::concurrency::BlockingQueue;

namespace {

void PushPopSingleItem()
{
    BlockingQueue<std::unique_ptr<int>> queue;
    queue.push(std::make_unique<int>(42));

    std::unique_ptr<int> value;
    assert(queue.pop(value));
    assert(value);
    assert(*value == 42);
    assert(queue.empty());
}

void PopBlocksUntilPush()
{
    BlockingQueue<int> queue;
    std::atomic<bool> popped = false;
    int value = 0;

    std::thread consumer([&] {
        bool result = queue.pop(value);
        assert(result);
        popped = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(!popped);

    queue.push(7);
    consumer.join();

    assert(popped);
    assert(value == 7);
}

void ShutdownWakesWaitingThread()
{
    BlockingQueue<int> queue;
    std::atomic<bool> returned = false;
    bool result = true;
    int value = 0;

    std::thread consumer([&] {
        result = queue.pop(value);
        returned = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(!returned);

    queue.shutdown();
    consumer.join();

    assert(returned);
    assert(!result);
}

void MultipleProducersConsumers()
{
    BlockingQueue<int> queue;
    constexpr int producerCount = 4;
    constexpr int consumerCount = 4;
    constexpr int itemsPerProducer = 250;
    constexpr int totalItems = producerCount * itemsPerProducer;

    std::atomic<int> consumedCount = 0;
    std::atomic<long long> consumedSum = 0;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    for (int i = 0; i < consumerCount; ++i) {
        consumers.emplace_back([&] {
            int value = 0;
            while (queue.pop(value)) {
                ++consumedCount;
                consumedSum += value;
            }
        });
    }

    long long expectedSum = 0;
    for (int producer = 0; producer < producerCount; ++producer) {
        producers.emplace_back([&, producer] {
            for (int item = 1; item <= itemsPerProducer; ++item) {
                queue.push((producer * itemsPerProducer) + item);
            }
        });

        for (int item = 1; item <= itemsPerProducer; ++item) {
            expectedSum += (producer * itemsPerProducer) + item;
        }
    }

    for (auto& producer : producers) {
        producer.join();
    }

    queue.shutdown();

    for (auto& consumer : consumers) {
        consumer.join();
    }

    assert(consumedCount == totalItems);
    assert(consumedSum == expectedSum);
    assert(queue.empty());
}

} // namespace

int main()
{
    PushPopSingleItem();
    PopBlocksUntilPush();
    ShutdownWakesWaitingThread();
    MultipleProducersConsumers();
    return 0;
}
