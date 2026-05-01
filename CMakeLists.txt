cmake_minimum_required(VERSION 3.20)

project(distributed-job-scheduler
    VERSION 0.1.0
    LANGUAGES CXX
)

add_library(scheduler_core INTERFACE)
target_compile_features(scheduler_core INTERFACE cxx_std_20)
target_include_directories(scheduler_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(scheduler_app app/main.cpp)
target_link_libraries(scheduler_app PRIVATE scheduler_core)

enable_testing()
