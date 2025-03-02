/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param payload: the message size in bytes
     */
    VirtualizedClusterComputeServiceMessage::VirtualizedClusterComputeServiceMessage(sg_size_t payload) : ComputeServiceMessage(payload) {
    }

    /**
     * @brief Constructor
     *
     * @param answer_commport: the commport to which to send the answer
     * @param vm_name: the name of the new VM host
     * @param dest_pm_hostname: the name of the destination physical machine host
     * @param payload: the message size in bytes
     *
     */
    VirtualizedClusterComputeServiceMigrateVMRequestMessage::VirtualizedClusterComputeServiceMigrateVMRequestMessage(
            S4U_CommPort *answer_commport,
            const std::string &vm_name,
            const std::string &dest_pm_hostname,
            sg_size_t payload) : VirtualizedClusterComputeServiceMessage(payload) {

#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_commport == nullptr) || dest_pm_hostname.empty() || vm_name.empty()) {
            throw std::invalid_argument(
                    "VirtualizedClusterComputeServiceMigrateVMRequestMessage::VirtualizedClusterComputeServiceMigrateVMRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_commport = answer_commport;
        this->vm_name = vm_name;
        this->dest_pm_hostname = dest_pm_hostname;
    }

    /**
     * @brief Constructor
     *
     * @param success: whether the VM migration was successful or not
     * @param failure_cause: a failure cause (or nullptr if success)
     * @param payload: the message size in bytes
     */
    VirtualizedClusterComputeServiceMigrateVMAnswerMessage::VirtualizedClusterComputeServiceMigrateVMAnswerMessage(
            bool success,
            std::shared_ptr<FailureCause> failure_cause,
            sg_size_t payload) : VirtualizedClusterComputeServiceMessage(payload), success(success),
                              failure_cause(std::move(failure_cause)) {}


}// namespace wrench
