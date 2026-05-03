cmake_minimum_required(VERSION 3.20)

project(distributed-job-scheduler
    VERSION 0.1.0
    LANGUAGES CXX
)

add_library(scheduler_core
    src/core/Job.cpp
    src/core/Task.cpp
    src/concurrency/ThreadPool.cpp
    src/execution/TaskExecutor.cpp
    src/scheduling/DependencyGraph.cpp
    src/scheduling/JobScheduler.cpp
    src/storage/InMemoryJobRepository.cpp
)
target_compile_features(scheduler_core PUBLIC cxx_std_20)
target_include_directories(scheduler_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(scheduler_app app/main.cpp)
target_link_libraries(scheduler_app PRIVATE scheduler_core)

enable_testing()
add_subdirectory(tests)
