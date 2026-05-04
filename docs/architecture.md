# Architecture

This project is currently a local C++20 job scheduler. It is structured into layers so distributed behavior can be added later without rewriting the core model.

## Core Layer

The core layer lives under `include/scheduler/core` and `src/core`.

It defines:

- `JobId` and `TaskId` as `std::uint64_t` aliases.
- `TaskStatus` and `JobStatus`.
- `Task`, which owns a task ID, name, status, retry counters, and `std::function<void()>` action.
- `Job`, which owns tasks and task dependency declarations.

`Job` rejects duplicate task IDs. Dependencies are stored as `task -> prerequisites`, where a task cannot run until its prerequisites complete.

## Concurrency Layer

The concurrency layer lives under `include/scheduler/concurrency` and `src/concurrency`.

It provides:

- `BlockingQueue<T>`: a header-only blocking queue using `std::queue<T>`, `std::mutex`, and `std::condition_variable`.
- `ThreadPool`: a fixed-size worker pool backed by `BlockingQueue<std::function<void()>>`.

`ThreadPool::shutdown()` stops accepting work, shuts down the queue, and joins workers. Submitting after shutdown throws `std::runtime_error`.

## Execution Layer

The execution layer lives under `include/scheduler/execution` and `src/execution`.

It provides:

- `ExecutionResult`, containing task ID, success flag, error message, and duration.
- `TaskExecutor`, which calls `Task::execute()`, measures duration, and converts exceptions into failure results.

The executor does not decide scheduling policy. It only runs one task and reports the result.

## Scheduling Layer

The scheduling layer lives under `include/scheduler/scheduling` and `src/scheduling`.

It provides:

- `DependencyGraph`, which tracks task readiness using indegree counts and prerequisite-to-dependent adjacency.
- `JobScheduler`, which coordinates repository updates, dependency readiness, thread pool submission, task execution, and job completion.

`JobScheduler` protects shared scheduler state with a mutex and uses a condition variable so `waitForJob()` blocks until a job reaches a terminal state.

## Storage Layer

The storage layer lives under `include/scheduler/storage` and `src/storage`.

It provides:

- `JobRepository`, an abstract interface.
- `InMemoryJobRepository`, a mutex-protected `std::unordered_map<JobId, Job>`.

The current repository is process-local and non-durable.

## Task Lifecycle

Current task lifecycle:

```text
Pending -> Running -> Succeeded
Pending -> Running -> Failed
```

When a job is submitted, tasks start as `Pending`. Ready tasks are marked `Running` before being submitted to the thread pool. After execution, successful tasks become `Succeeded`; tasks that throw become `Failed`.

Retries are not scheduled yet, even though `Task` already stores retry counters.

## Dependency Scheduling

Dependency pairs are interpreted as:

```text
task depends on prerequisite
```

For example:

```text
task 4 depends on task 2
task 4 depends on task 3
```

Internally, `DependencyGraph` stores the reverse direction:

```text
prerequisite -> dependent tasks
```

This allows the scheduler to quickly find newly ready tasks when a prerequisite completes.

Scheduling flow:

1. `JobScheduler::submitJob()` saves the job.
2. It validates dependencies and builds a dependency graph.
3. If the graph has a cycle, the job is marked `Failed`.
4. Tasks with zero prerequisites are scheduled first.
5. When a task succeeds, the graph decrements dependent indegrees.
6. Dependents whose indegree reaches zero are scheduled.
7. The job becomes `Succeeded` when all tasks succeed.
8. The job becomes `Failed` when any task fails.
