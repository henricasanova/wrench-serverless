/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/JobTimeout.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/job/Job.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_job_timeout, "Log category for JobTimeout");

namespace wrench {

    /**
    * @brief Constructor
    *
    * @param job: the job that has timed out
    */
    JobTimeout::JobTimeout(std::shared_ptr<Job> job) {
        this->job = std::move(job);
    }


    /**
     * @brief Getter
     * @return the job
     */
    std::shared_ptr<Job> JobTimeout::getJob() {
        return this->job;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string JobTimeout::toString() {
        return std::string("Job has timed out - likely not enough time was requested for batch job");
    }

}// namespace wrench
