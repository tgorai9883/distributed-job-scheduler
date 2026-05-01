#include "scheduler/storage/InMemoryJobRepository.hpp"

namespace scheduler::storage {

void InMemoryJobRepository::saveJob(const core::Job& job)
{
    std::lock_guard<std::mutex> lock(mutex_);
    jobs_.insert_or_assign(job.id(), job);
}

std::optional<core::Job> InMemoryJobRepository::getJob(core::JobId jobId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = jobs_.find(jobId);
    if (it == jobs_.end()) {
        return std::nullopt;
    }

    return it->second;
}

void InMemoryJobRepository::updateJob(const core::Job& job)
{
    std::lock_guard<std::mutex> lock(mutex_);
    jobs_.insert_or_assign(job.id(), job);
}

bool InMemoryJobRepository::exists(core::JobId jobId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return jobs_.find(jobId) != jobs_.end();
}

} // namespace scheduler::storage
