#include "scheduler/distributed/CoordinatorServer.hpp"

#include <chrono>
#include <iostream>
#include <string>

int main()
{
    scheduler::distributed::CoordinatorServer coordinator;

    std::cout << "Coordinator started in text-protocol mode\n";
    std::cout << "Enter messages like: REGISTER worker-1 or HEARTBEAT worker-1\n";

    std::string line;
    while (std::getline(std::cin, line)) {
        coordinator.markDeadWorkers(std::chrono::seconds(5));
        std::cout << coordinator.handleMessage(line) << '\n';
    }

    return 0;
}
