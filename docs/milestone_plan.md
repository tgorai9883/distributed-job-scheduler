# Milestone Plan

## Completed Milestones

- Initial C++20 project skeleton with CMake, scripts, docs, examples, and tests folders.
- Core model layer:
  - `JobId` and `TaskId`
  - `TaskStatus` and `JobStatus`
  - `Task`
  - `Job`
- Storage layer:
  - `JobRepository` interface
  - mutex-protected `InMemoryJobRepository`
- Concurrency layer:
  - header-only `BlockingQueue<T>`
  - fixed-size `ThreadPool`
- Execution layer:
  - `ExecutionResult`
  - `TaskExecutor`
- Scheduling layer:
  - `DependencyGraph`
  - `JobScheduler`
- Failure behavior:
  - task exception fails the task and job
  - dependency cycles fail the job
  - invalid dependencies fail the job
  - duplicate task IDs are rejected
  - submit after shutdown throws
  - unknown jobs and tasks throw on lookup/wait
- Example programs:
  - simple independent job
  - dependent task graph
  - failure handling
- Test coverage for queue, thread pool, dependency graph, scheduler, and failure cases.

## Future Milestones

- Task retry scheduling using existing retry metadata.
- Task cancellation and timeout handling.
- Durable repository implementation.
- Priority or resource-aware scheduling.
- Remote worker execution.
- Worker heartbeats, leases, and failure recovery.
- Distributed coordination for task ownership and completion.
