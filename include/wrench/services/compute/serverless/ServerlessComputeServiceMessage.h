/**
* Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SERVERLESSCOMPUTESERVICEMESSAGE_H
#define SERVERLESSCOMPUTESERVICEMESSAGE_H

#include "wrench/services/compute/ComputeServiceMessage.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a BatchComputeService
     */
    class ServerlessComputeServiceMessage : public ComputeServiceMessage {
    protected:
        ServerlessComputeServiceMessage(sg_size_t payload);
    };

    /**
     * @brief A message sent to a ServerlessComputeService to register a function
     */
    class ServerlessComputeServiceFunctionRegisterRequestMessage : public ServerlessComputeServiceMessage {
    public:
        ServerlessComputeServiceFunctionRegisterRequestMessage(S4U_CommPort *answer_commport, std::shared_ptr<Function> function, double time_limit, sg_size_t disk_space_limit_in_bytes, sg_size_t ram_limit_in_bytes, sg_size_t ingress_in_bytes, sg_size_t egress_in_bytes, sg_size_t payload);

        /** @brief The commport_name to answer to */
        S4U_CommPort *answer_commport;
        /** @brief The function to register */
        std::shared_ptr<Function> function;
        /** @brief The time limit for execution */
        double time_limit_in_seconds;
        /** @brief Disk space limit for the function in bytes */
        sg_size_t disk_space_limit_in_bytes;
        /** @brief RAM limit for the function in bytes */
        sg_size_t ram_limit_in_bytes;
        /** @brief Ingress data limit in bytes */
        sg_size_t ingress_in_bytes;
        /** @brief Egress data limit in bytes */
        sg_size_t egress_in_bytes;

    };

    /**
     * @brief A message sent from a ServerlessComputeService in reply to a function registration request
     */
    class ServerlessComputeServiceFunctionRegisterAnswerMessage : public ServerlessComputeServiceMessage {
    public:
        ServerlessComputeServiceFunctionRegisterAnswerMessage(bool success);

        /** @brief Whether the registration was successtul */
        bool success;
        // TODO: Add a Failure Cause
    };

    /***********************/
    /** \endcond           */
    /***********************/

}



#endif //SERVERLESSCOMPUTESERVICEMESSAGE_H
