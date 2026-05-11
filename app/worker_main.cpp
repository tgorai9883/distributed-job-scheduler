#include "scheduler/distributed/CoordinatorServer.hpp"
#include "scheduler/distributed/WorkerClient.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv)
{
    const std::string workerId = argc > 1 ? argv[1] : "worker-1";

    scheduler::distributed::CoordinatorServer coordinator;
    scheduler::distributed::WorkerClient worker(workerId, coordinator);

    worker.registerWithCoordinator();
    worker.sendHeartbeat();

    std::cout << "Worker " << workerId << " registered and sent one heartbeat\n";
    std::cout << "Standalone networking is not implemented yet; this is an in-process protocol demo\n";

    return 0;
}
