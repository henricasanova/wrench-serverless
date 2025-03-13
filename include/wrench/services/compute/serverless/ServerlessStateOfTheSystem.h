/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <vector>
#include <map>
#include <queue>
#include <set>
#include <memory>
#include <string>
#include <wrench/services/compute/serverless/Invocation.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/storage/DataFile.h>

#ifndef WRENCH_SERVERLESSSTATEOFTHESYSTEM_H
#define WRENCH_SERVERLESSSTATEOFTHESYSTEM_H
 
namespace wrench {
 
    class ServerlessStateOfTheSystem {
    private:

        friend class ServerlessComputeService;

        explicit StateOfTheSystem(std::vector<std::string> compute_hosts);

        // map of Registered functions sorted by function name
        std::map<std::string, std::shared_ptr<RegisteredFunction>> _registeredFunctions;
        // vector of compute host names
        std::vector<std::string> _compute_hosts;

        // map of available cores on each compute host
        std::map<std::string, unsigned long> _available_cores;
        // map of scheduling decisions for each invocation
        std::map<std::shared_ptr<Invocation>, std::string> _scheduling_decisions;

        // queue of function invocations waiting to be processed
        std::queue<std::shared_ptr<Invocation>> _newInvocations;
        // queues of function invocations whose images are being downloaded
        std::map<std::shared_ptr<DataFile>, std::queue<std::shared_ptr<Invocation>>> _admittedInvocations;
        // queue of function invocations whose images have been downloaded
        std::queue<std::shared_ptr<Invocation>> _schedulableInvocations;
        // queue of function invocations whose are scheduled on a host and whose
        // images are being copied there
        std::queue<std::shared_ptr<Invocation>> _scheduledInvocations;
        // queue of function invocations currently running
        std::queue<std::shared_ptr<Invocation>> _runningInvocations;
        // queue of function invocations that have finished executing
        std::queue<std::shared_ptr<Invocation>> _finishedInvocations;

        std::string _head_storage_service_mount_point;
        // std::vector<std::shared_ptr<BareMetalComputeService>> _compute_services;
        std::unordered_map<std::string, std::shared_ptr<StorageService>> _compute_storages;
        std::shared_ptr<StorageService> _head_storage_service;
        std::set<std::shared_ptr<DataFile>> _being_downloaded_image_files;
        std::set<std::shared_ptr<DataFile>> _downloaded_image_files;
        sg_size_t _free_space_on_head_storage; // We keep track of it ourselves to avoid concurrency shennanigans

        ~StateOfTheSystem();

    public:
    
        

    }

 
} // namespace wrench
 
#endif // WRENCH_SERVERLESSSTATEOFTHESYSTEM_H
 