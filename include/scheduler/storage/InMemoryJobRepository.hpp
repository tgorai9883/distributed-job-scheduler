#pragma once

#include "scheduler/storage/JobRepository.hpp"

#include <mutex>
#include <unordered_map>

namespace scheduler::storage {

class InMemoryJobRepository : public JobRepository {
public:
    void saveJob(const core::Job& job) override;
    [[nodiscard]] std::optional<core::Job> getJob(core::JobId jobId) override;
    void updateJob(const core::Job& job) override;
    [[nodiscard]] bool exists(core::JobId jobId) const override;

private:
    mutable std::mutex mutex_;
    std::unordered_map<core::JobId, core::Job> jobs_;
};

} // namespace scheduler::storage
