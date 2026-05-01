#pragma once

#include "scheduler/core/Job.hpp"
#include "scheduler/core/SchedulerTypes.hpp"

#include <optional>

namespace scheduler::storage {

class JobRepository {
public:
    virtual ~JobRepository() = default;

    virtual void saveJob(const core::Job& job) = 0;
    [[nodiscard]] virtual std::optional<core::Job> getJob(core::JobId jobId) = 0;
    virtual void updateJob(const core::Job& job) = 0;
    [[nodiscard]] virtual bool exists(core::JobId jobId) const = 0;
};

} // namespace scheduler::storage
