/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CLOUDSERVICEMESSAGEPAYLOAD_H
#define WRENCH_CLOUDSERVICEMESSAGEPAYLOAD_H

#include "wrench/services/compute/ComputeServiceMessagePayload.h"

namespace wrench {

    /**
     * @brief Configurable message payloads for a CloudService
     */
    class CloudServiceMessagePayload : public ComputeServiceMessagePayload {

    public:
        /** @brief The number of bytes in the control message sent to the service to request a get execution hosts. **/
        DECLARE_MESSAGEPAYLOAD_NAME(GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the service in answer to a get execution hosts request. **/
        DECLARE_MESSAGEPAYLOAD_NAME(GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the service to request a VM creation. **/
        DECLARE_MESSAGEPAYLOAD_NAME(CREATE_VM_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the service in answer to a VM creation request. **/
        DECLARE_MESSAGEPAYLOAD_NAME(CREATE_VM_ANSWER_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent to the service to request a VM shutdown. **/
        DECLARE_MESSAGEPAYLOAD_NAME(SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the service in answer to a VM shutdown request. **/
        DECLARE_MESSAGEPAYLOAD_NAME(SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD);
    };
}

#endif //WRENCH_CLOUDSERVICEMESSAGEPAYLOAD_H
