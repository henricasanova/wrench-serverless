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

#include "Invocation.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/failure_causes/FailureCause.h"
#include "wrench/managers/function_manager/Function.h"

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
        ServerlessComputeServiceFunctionRegisterAnswerMessage(bool success, std::shared_ptr<RegisteredFunction> registered_function, std::shared_ptr<FailureCause> failure_cause, sg_size_t payload);

        /** @brief Whether the registration was successful */
        bool success;
        /** @brief The registered function on success or nullptr on failure */
        std::shared_ptr<RegisteredFunction> registered_function;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a ServerlessComputeService to invoke a function
     */
    class ServerlessComputeServiceFunctionInvocationRequestMessage : public ServerlessComputeServiceMessage {
    public:
        ServerlessComputeServiceFunctionInvocationRequestMessage(S4U_CommPort *answer_commport, const std::shared_ptr<RegisteredFunction>& registered_function, const std::shared_ptr<FunctionInput>& function_input, S4U_CommPort *notify_commport, sg_size_t payload);

        /** @brief The commport_name to answer to */
        S4U_CommPort *answer_commport;
        /** @brief The function to invoke */
        std::shared_ptr<RegisteredFunction> registered_function;
        /** @brief   */
        std::shared_ptr<FunctionInput> function_input;
        /** @brief The commport_name to send notifications to */
        S4U_CommPort *notify_commport;
    };

    /**
     * @brief A message sent from a ServerlessComputeService in reply to a function invocation request
     */
    class ServerlessComputeServiceFunctionInvocationAnswerMessage : public ServerlessComputeServiceMessage {
    public:
        ServerlessComputeServiceFunctionInvocationAnswerMessage(bool success, std::shared_ptr<Invocation> invocation, std::shared_ptr<FailureCause> failure_cause, sg_size_t payload);

        /** @brief Whether the invocation will be completed or not at some point in the future*/
        bool success;
        /** @brief The invocation object **/
        std::shared_ptr<Invocation> invocation;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent from a ServerlessComputeService when a function invocation is completed
     */
    class ServerlessComputeServiceFunctionInvocationCompleteMessage : public ServerlessComputeServiceMessage {
    public:
        ServerlessComputeServiceFunctionInvocationCompleteMessage(bool success, std::shared_ptr<Invocation> invocation, std::shared_ptr<FailureCause> failure_cause, sg_size_t payload);

        /** @brief Whether the invocation was successful */
        bool success;
        /** @brief The invocation object */
        std::shared_ptr<Invocation> invocation;
        /** @brief The cause of the failure, or nullptr on success */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
     * @brief A message sent to a ServerlessComputeService to notify it that an image download has completed
     */
    class ServerlessComputeServiceDownloadCompleteMessage : public ServerlessComputeServiceMessage {
    public:
        ServerlessComputeServiceDownloadCompleteMessage(std::shared_ptr<CustomAction> action, std::shared_ptr<DataFile> image_file, sg_size_t payload);

        /** @brief The action that did the download */
        std::shared_ptr<CustomAction> _action;
        /** @brief The invocation the download was for */
        std::shared_ptr<DataFile> _image_file;
    };

    /**
     * @brief A message sent to a ServerlessComputeService to notify it that an image download has completed
     */
    class ServerlessComputeServiceInvocationExecutionCompleteMessage : public ServerlessComputeServiceMessage {
    public:
        ServerlessComputeServiceInvocationExecutionCompleteMessage(std::shared_ptr<CustomAction> action, std::shared_ptr<Invocation> invocation, sg_size_t payload);

        /** @brief The action that did the download */
        std::shared_ptr<CustomAction> _action;
        /** @brief The invocation the download was for */
        std::shared_ptr<Invocation> _invocation;
    };

    class ServerlessComputeServiceNodeCopyCompleteMessage : public ServerlessComputeServiceMessage {
    public:
        ServerlessComputeServiceNodeCopyCompleteMessage(std::shared_ptr<CustomAction> action, std::shared_ptr<DataFile> image_file, std::string compute_host, sg_size_t payload);
        /** @brief The action that did the copy */
        std::shared_ptr<CustomAction> _action;
        /** @brief The image that was copied */
        std::shared_ptr<DataFile> _image_file;
        /** @brief The compute host on which the image was copied to */
        std::string _compute_host;
    };

    class ServerlessComputeServiceNodeLoadCompleteMessage : public ServerlessComputeServiceMessage {
    public:
        ServerlessComputeServiceNodeLoadCompleteMessage(std::shared_ptr<CustomAction> action, std::shared_ptr<DataFile> image_file, std::string compute_host, sg_size_t payload);
        /** @brief The action that did the copy */
        std::shared_ptr<CustomAction> _action;
        /** @brief The image that was loaded */
        std::shared_ptr<DataFile> _image_file;
        /** @brief The compute host at which the image was  loaded */
        std::string _compute_host;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}



#endif //SERVERLESSCOMPUTESERVICEMESSAGE_H
