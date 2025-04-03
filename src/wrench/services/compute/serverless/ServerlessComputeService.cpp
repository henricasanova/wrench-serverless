/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/serverless/ServerlessComputeService.h>
#include <wrench/services/compute/serverless/ServerlessComputeServiceMessage.h>
#include <wrench/services/compute/serverless/ServerlessComputeServiceMessagePayload.h>
#include <wrench/services/compute/serverless/Invocation.h>
#include <wrench/managers/function_manager/Function.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/failure_causes/NotAllowed.h>
#include <wrench/failure_causes/FunctionNotFound.h>

#include <utility>

#include "wrench/action/CustomAction.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/helper_services/action_executor/ActionExecutor.h"
#include "wrench/services/storage/simple/SimpleStorageService.h"
#include "wrench/simulation/Simulation.h"

WRENCH_LOG_CATEGORY(wrench_core_serverless_service, "Log category for Serverless Compute Service");

namespace wrench {
    ServerlessComputeService::ServerlessComputeService(const std::string& hostname,
                                                           std::vector<std::string> compute_hosts,
                                                           std::string head_storage_service_mount_point,
                                                           std::shared_ptr<ServerlessScheduler> scheduler,
                                                           WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                           WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list) :
        ComputeService(hostname,
                       "ServerlessComputeService", "") {

        _state_of_the_system = std::shared_ptr<ServerlessStateOfTheSystem>(
            new ServerlessStateOfTheSystem(std::move(compute_hosts)));

        _state_of_the_system->_head_storage_service_mount_point = std::move(head_storage_service_mount_point);

        _scheduler = scheduler;

        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);

        // Set default and specified properties
        this->setProperties(this->default_property_values, property_list);
        
    }

    /**
     * @brief Returns true if the service supports standard jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsStandardJobs() {
        return false;
    }

    /**
     * @brief Returns true if the service supports compound jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsCompoundJobs() {
        return false;
    }

    /**
     * @brief Returns true if the service supports pilot jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsPilotJobs() {
        return false;
    }

    /**
     * @brief Method to submit a compound job to the service
     *
     * @param job: The job being submitted
     * @param service_specific_args: the set of service-specific arguments
     */
    void ServerlessComputeService::submitCompoundJob(std::shared_ptr<CompoundJob> job,
                                                     const std::map<std::string, std::string>& service_specific_args) {
        throw std::runtime_error("ServerlessComputeService::submitCompoundJob: should not be called");
    }

    /**
     * @brief Method to terminate a compound job at the service
     *
     * @param job: The job being submitted
     */
    void ServerlessComputeService::terminateCompoundJob(std::shared_ptr<CompoundJob> job) {
        throw std::runtime_error("ServerlessComputeService::terminateCompoundJob: should not be called");
    }

    /**
     * @brief Construct a dict for resource information
     * @param key: the desired key
     * @return a dictionary
     */
    std::map<std::string, double> ServerlessComputeService::constructResourceInformation(const std::string& key) {
        throw std::runtime_error("ServerlessComputeService::constructResourceInformation: not implemented");
    }

    /**
     * @brief Register a function in the serverless compute service
     *
     * @param function the function to register
     * @param time_limit_in_seconds the time limit for execution
     * @param disk_space_limit_in_bytes the disk space limit for the function
     * @param RAM_limit_in_bytes the RAM limit for the function
     * @param ingress_in_bytes the ingress data limit
     * @param egress_in_bytes the egress data limit
     * @return true if the function was registered successfully
     * @throw ExecutionException if the function registration fails
     */
    bool ServerlessComputeService::registerFunction(const std::shared_ptr<Function>& function, double time_limit_in_seconds,
                                                    sg_size_t disk_space_limit_in_bytes, sg_size_t RAM_limit_in_bytes,
                                                    sg_size_t ingress_in_bytes, sg_size_t egress_in_bytes) {
        WRENCH_INFO("Serverless Provider Registered function %s", function->getName().c_str());
        auto answer_commport = S4U_CommPort::getTemporaryCommPort();

        //  send a "run a standard job" message to the daemon's commport
        this->commport->putMessage(
            new ServerlessComputeServiceFunctionRegisterRequestMessage(
                answer_commport, function, time_limit_in_seconds, disk_space_limit_in_bytes, RAM_limit_in_bytes,
                ingress_in_bytes, egress_in_bytes,
                this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        auto msg = answer_commport->getMessage<ServerlessComputeServiceFunctionRegisterAnswerMessage>(
            this->network_timeout,
            "ServerlessComputeService::registerFunction(): Received an");

        // TODO: Deal with failures later
        if (not msg->success) {
            throw ExecutionException(msg->failure_cause);
        }
        return true;
    }

    /**
     * @brief Invoke a function in the serverless compute service
     *
     * @param function the function to invoke
     * @param input the input to the function
     * @param notify_commport the ExecutionController commport to notify
     * @return std::shared_ptr<Invocation> Pointer to the invocation created by the ServerlessComputeService
     */
    std::shared_ptr<Invocation> ServerlessComputeService::invokeFunction(
        std::shared_ptr<Function> function, std::shared_ptr<FunctionInput> input, S4U_CommPort* notify_commport) {
        WRENCH_INFO("Serverless Provider received invoke function %s", function->getName().c_str());
        auto answer_commport = S4U_CommPort::getTemporaryCommPort();
        this->commport->dputMessage(
            new ServerlessComputeServiceFunctionInvocationRequestMessage(answer_commport,
                                                                         function, input,
                                                                         notify_commport, 0));

        // Block here for return, if non-blocking then function manager has to check up on it? or send a message
        auto msg = answer_commport->getMessage<ServerlessComputeServiceFunctionInvocationAnswerMessage>(
            this->network_timeout,
            "ServerlessComputeService::invokeFunction(): Received an");

        if (not msg->success) {
            throw ExecutionException(msg->failure_cause);
        }
        return msg->invocation;
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int ServerlessComputeService::main() {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        WRENCH_INFO("Serverless provider starting (%s)", this->commport->get_cname());

        // Start the Head Storage Service
        startHeadStorageService();
        // Start a storage service on each host.
        startComputeHostsServices();

        while (processNextMessage()) {
            admitInvocations();
            scheduleInvocations();
            dispatchInvocations();
        }
        return 0;
    }

    /**
     * @brief Process the next message in the commport
     *
     * @return true if the ServerlessComputeService daemon should continue processing messages
     * @return false if the ServerlessComputeService daemon should die
     */
    bool ServerlessComputeService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        }
        catch (ExecutionException& e) {
            WRENCH_INFO(
                "Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto ss_mesg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // TODO: Die...
            return false;
        }
        else if (auto scsfrr_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceFunctionRegisterRequestMessage>(message)) {
            processFunctionRegistrationRequest(
                scsfrr_msg->answer_commport, scsfrr_msg->function, scsfrr_msg->time_limit_in_seconds,
                scsfrr_msg->disk_space_limit_in_bytes, scsfrr_msg->ram_limit_in_bytes,
                scsfrr_msg->ingress_in_bytes, scsfrr_msg->egress_in_bytes);
            return true;
        }
        else if (auto scsfir_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceFunctionInvocationRequestMessage>(message)) {
            processFunctionInvocationRequest(scsfir_msg->answer_commport, scsfir_msg->function,
                                             scsfir_msg->function_input, scsfir_msg->notify_commport);
            return true;
        }
        else if (auto scsdc_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceDownloadCompleteMessage>(message)) {
            processImageDownloadCompletion(scsdc_msg->_action, scsdc_msg->_image_file);
            return true;
        }
        else if (auto scsiec_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceInvocationExecutionCompleteMessage>(message)) {
            auto invocation = scsiec_msg->_invocation;
            auto host = _scheduling_decisions[invocation];
            _scheduling_decisions.erase(invocation);
            _available_cores[host]++;
            scsiec_msg->_invocation->_notify_commport->dputMessage(
                new ServerlessComputeServiceFunctionInvocationCompleteMessage(true,
                                                                              scsiec_msg->_invocation, nullptr, 0));
            return true;
        }
        else if (auto scsncc_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceNodeCopyCompleteMessage>(message)) {
            // Do the main loop again to schedule the invocation?
            return true;
        }
        else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Processes a "function registration request" message
     *
     * @param answer_commport the FunctionManager commport to answer to
     * @param function the function to register
     * @param time_limit the time limit for execution
     * @param disk_space_limit_in_bytes the disk space limit for the function
     * @param ram_limit_in_bytes the RAM limit for the function
     * @param ingress_in_bytes the ingress data limit
     * @param egress_in_bytes the egress data limit
     */
    void ServerlessComputeService::processFunctionRegistrationRequest(S4U_CommPort* answer_commport,
                                                                      std::shared_ptr<Function> function,
                                                                      double time_limit,
                                                                      sg_size_t disk_space_limit_in_bytes,
                                                                      sg_size_t ram_limit_in_bytes,
                                                                      sg_size_t ingress_in_bytes,
                                                                      sg_size_t egress_in_bytes) {
        if (_state_of_the_system->_registeredFunctions.find(function->getName()) != _state_of_the_system->_registeredFunctions.end()) {
            // TODO: Create failure case for duplicate function?
            std::string msg = "Duplicate Function";
            auto answerMessage =
                new ServerlessComputeServiceFunctionRegisterAnswerMessage(
                    false, function,
                    std::make_shared<NotAllowed>(this->getSharedPtr<ServerlessComputeService>(), msg), 0);
            answer_commport->dputMessage(answerMessage);
        }
        else {
            _state_of_the_system->_registeredFunctions[function->getName()] = std::make_shared<RegisteredFunction>(
                function,
                time_limit,
                disk_space_limit_in_bytes,
                ram_limit_in_bytes,
                ingress_in_bytes,
                egress_in_bytes);
            auto answerMessage = new ServerlessComputeServiceFunctionRegisterAnswerMessage(true, function, nullptr, 0);
            answer_commport->dputMessage(answerMessage);
        }
    }

    /**
     * @brief Processes a "function invocation request" message
     *
     * @param answer_commport the FunctionManager commport to answer to
     * @param function the function to invoke
     * @param input the input to the function
     * @param notify_commport the ExecutionController commport to notify
     */
    void ServerlessComputeService::processFunctionInvocationRequest(S4U_CommPort* answer_commport,
                                                                    std::shared_ptr<Function> function,
                                                                    std::shared_ptr<FunctionInput> input,
                                                                    S4U_CommPort* notify_commport) {
        if (_state_of_the_system->_registeredFunctions.find(function->getName()) == _state_of_the_system->_registeredFunctions.end()) {
            // Not found
            const auto answerMessage = new ServerlessComputeServiceFunctionInvocationAnswerMessage(
                false, nullptr, std::make_shared<FunctionNotFound>(function), 0);
            answer_commport->dputMessage(answerMessage);
        }
        else {
            const auto invocation = std::make_shared<Invocation>(_state_of_the_system->_registeredFunctions.at(function->getName()), input,
                                                                 notify_commport);
            _state_of_the_system->_newInvocations.push(invocation);
            // TODO: return some sort of function invocation object?
            const auto answerMessage = new ServerlessComputeServiceFunctionInvocationAnswerMessage(
                true, invocation, nullptr, 0);
            answer_commport->dputMessage(answerMessage);
        }
    }

    /**
     * @brief Processes an "image download completion" message
     *
     * @param action to get failure cause from
     * @param image_file The image file that was downloaded, used as key to map downloading functions
     */
    void ServerlessComputeService::processImageDownloadCompletion(const std::shared_ptr<Action>& action,
                                                                  const std::shared_ptr<DataFile>& image_file) {
        if (action->getFailureCause()) {
            throw std::runtime_error("ServerlessComputeService::processImageDownloadCompletion(): "
                "An image download (from remote) has failed. Handling of such failures is currently not implemented");
        }
        WRENCH_INFO("ServerlessComputeService::processImageDownloadCompletion(): Image file %s was downloaded",
                    image_file->getID().c_str());
        _state_of_the_system->_being_downloaded_image_files.erase(image_file);
        _state_of_the_system->_downloaded_image_files.insert(image_file);

        // Move all relevant invocations from the admitted to the schedulable queue
        auto& queue = _state_of_the_system->_admittedInvocations[image_file];
        while (not queue.empty()) {
            _state_of_the_system->_schedulableInvocations.push(std::move(queue.front()));
            queue.pop();
        }
        _state_of_the_system->_admittedInvocations.erase(image_file);
    }

    /**
     * @brief Dispatches scheduled function invocations to compute hosts
     *
     */
    void ServerlessComputeService::dispatchInvocations() {
        while (!_state_of_the_system->_scheduledInvocations.empty()) {
            auto invocation_to_place = _state_of_the_system->_scheduledInvocations.front();
            WRENCH_INFO("Invoking function [%s]",
                        invocation_to_place->_registered_function->_function->getName().c_str());
            _state_of_the_system->_scheduledInvocations.pop();

            dispatchFunctionInvocation(invocation_to_place);
        }
    }

    /**
     *
     * @param invocation : invocation to dispatch
     */
    void ServerlessComputeService::dispatchFunctionInvocation(const std::shared_ptr<Invocation>& invocation) {
        auto target_host = _state_of_the_system->_scheduling_decisions[invocation];

        auto ss = startInvocationStorageService(invocation);

        const std::function lambda_terminate = [](const std::shared_ptr<ActionExecutor>& action_executor) {
        };

        const std::function lambda_execute = [invocation, target_host, this](const std::shared_ptr<ActionExecutor>& action_executor) {
            WRENCH_INFO("In the function invocation lambda execute!!");
            auto function = invocation->_registered_function->_function;
            auto image_file = function->_image->getFile();
            auto local_image_path = wrench::FileLocation::LOCATION(_state_of_the_system->_compute_storages[target_host], image_file);

            // Simulate the reading of the image from disk into ram to spin up the container
            StorageService::readFileAtLocation(local_image_path);

            // Simulate the git-clone by copying from the remote location to the tmp storage service
            auto code_file = function->_code->getFile();
            StorageService::copyFile(invocation->_registered_function->_function->_code,
                wrench::FileLocation::LOCATION(invocation->_tmp_storage_service, code_file));

            // Invoke the user's lambda function
            function->_lambda(invocation->_function_input, invocation->_tmp_storage_service);

            // Clean up
            invocation->_tmp_storage_service->stop();
            invocation->_tmp_storage_service = nullptr; // Should free up all memory...
            StorageService::removeFileAtLocation(invocation->_tmp_file);

            WRENCH_INFO("Done with the lambda execute!!");
        };

        // Create the action and run it in an action executor
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "run_invocation_" + invocation->_registered_function->_function->getName(),
                0, 0, lambda_execute, lambda_terminate));

        auto custom_message = new ServerlessComputeServiceInvocationExecutionCompleteMessage(
            action,
            invocation, 0);

        auto action_executor = std::make_shared<ActionExecutor>(
            target_host,
            1,
            0,
            0,
            false,
            this->commport,
            custom_message,
            action,
            nullptr);

        action_executor->setSimulation(this->simulation_);
        WRENCH_INFO("Starting an action executor...");
        action_executor->start(action_executor, true, false);

        _state_of_the_system->_runningInvocations.push(invocation);
        WRENCH_INFO("Function [%s] invoked",
                    invocation->_registered_function->_function->getName().c_str());
        // invocation_to_place->output = whatever
        // invocation_to_place->_registered_function->_function->run_lambda()
    }

    /**
     * @brief Start a SimpleStorageService for each compute host. We don't start a bare-metal
     *        service as we'll do everything ourselves with action executor services.
     */
    void ServerlessComputeService::startComputeHostsServices() {
        for (auto const& host : _state_of_the_system->_compute_hosts) {
            if (not S4U_Simulation::hostHasMountPoint(host, "/")) {
                throw std::invalid_argument("ServerlessComputeService::startComputeHostsServices(): "
                    "each compute host in a serverless compute service should have a \"/\" mountpoint");
            }
            auto ss = this->simulation_->startNewService(SimpleStorageService::createSimpleStorageService(
                host,
                {"/"},
                {},
                {}));
            ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
            _state_of_the_system->_compute_storages[host] = ss;
        }
    }

    /**
     * @brief Method to create a tmp storage service for an invocation
     *
     * @param invocation A function invocation
     * @return A storage service
     */
    std::shared_ptr<StorageService> ServerlessComputeService::startInvocationStorageService(const std::shared_ptr<Invocation>& invocation) {
        static unsigned long seq = -1;
        seq++;
        auto host = _state_of_the_system->_scheduling_decisions[invocation];

        WRENCH_INFO("Starting a new storage service for an invocation...");
        // Reserve space on the storage service
        auto tmp_file = wrench::FileLocation::LOCATION(_state_of_the_system->_compute_storages[host],
            Simulation::addFile("tmp_" + std::to_string(seq), invocation->_registered_function->_disk_space));
        StorageService::createFileAtLocation(tmp_file);

        // Create a tmp file system
        auto disk = S4U_Simulation::hostHasMountPoint(host, "/");
        auto ods = simgrid::fsmod::OneDiskStorage::create("is_" + std::to_string(seq), disk);
        auto fs = simgrid::fsmod::FileSystem::create("fs" + std::to_string(seq));
        fs->mount_partition("/", ods, invocation->_registered_function->_disk_space);

        // Create a tmp storage service
        auto ss = std::shared_ptr<SimpleStorageService>(
            SimpleStorageService::createSimpleStorageServiceWithExistingFileSystem(host, fs, {}, {}));
        ss->setSimulation(this->simulation_);
        ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
        ss->start(ss, true, false);

        // Keep track of all this
        invocation->_tmp_file = tmp_file;
        invocation->_tmp_storage_service = ss;

        return ss;
    }

    /**
     * @brief
     *
     */
    void ServerlessComputeService::startHeadStorageService() {
        auto ss = SimpleStorageService::createSimpleStorageService(
            hostname,
            {_state_of_the_system->_head_storage_service_mount_point},
            {
                {
                    wrench::SimpleStorageServiceProperty::BUFFER_SIZE,
                    this->getPropertyValueAsString(ComputeServiceProperty::SCRATCH_SPACE_BUFFER_SIZE)
                }
            },
            {});
        ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
        ss->setSimulation(this->simulation_);
        _state_of_the_system->_head_storage_service = this->simulation_->startNewService(ss);
        _state_of_the_system->_free_space_on_head_storage = _state_of_the_system->_head_storage_service->getTotalSpace();
    }

    /**
     * @brief
     *
     */
    void ServerlessComputeService::admitInvocations() {
        // This implements a FCFS algorithm. That is, if an invocation is placed for an image
        // that cannot be downloaded right now (due to lack of space), then we stop and do not
        // consider invocations that were placed later, even if their images have been downloaded
        // and are available right now. This is an arbitrary non-backfilling choice, that can later
        // be revisited (e.g., creating a property that allows the user to pick one of several
        // strategies).

        while (!_state_of_the_system->_newInvocations.empty()) {
            WRENCH_INFO("Admitting an invocation...");
            auto invocation = _state_of_the_system->_newInvocations.front();
            auto image = invocation->_registered_function->_function->_image;

            // If the image file is already downloaded, make the invocation schedulable immediately
            if (_state_of_the_system->_downloaded_image_files.find(image->getFile()) != _state_of_the_system->_downloaded_image_files.end()) {
                _state_of_the_system->_newInvocations.pop();
                _state_of_the_system->_schedulableInvocations.push(invocation);
                continue;
            }

            // If the image file is being downloaded, make the invocation admitted
            if (_state_of_the_system->_being_downloaded_image_files.find(image->getFile()) != _state_of_the_system->_being_downloaded_image_files.end()) {
                _state_of_the_system->_newInvocations.pop();
                _state_of_the_system->_admittedInvocations[image->getFile()].push(invocation);
                continue;
            }

            // Otherwise, if there is enough space on the head node storage service to store it,
            // then launch the downloaded and admit the invocation
            if (_state_of_the_system->_free_space_on_head_storage >= image->getFile()->getSize()) {
                // "Reserve" space on the storage service
                _state_of_the_system->_free_space_on_head_storage -= image->getFile()->getSize();
                // initiate the download
                _state_of_the_system->_being_downloaded_image_files.insert(image->getFile());
                initiateImageDownloadFromRemote(invocation);
                _state_of_the_system->_newInvocations.pop();
                _state_of_the_system->_admittedInvocations[image->getFile()].push(invocation);
                continue;
            }

            // If we're here, we couldn't admit invocations, and so we stop
            break;
        }
    }

    /**
     * @brief
     *
     * @param invocation
     */
    void ServerlessComputeService::initiateImageDownloadFromRemote(const std::shared_ptr<Invocation>& invocation) {
        // Create a custom action (we could use a simple FileCopyAction here, but we are using a CustomAction
        // to demonstrate its use)
        const std::function lambda_execute = [invocation, this](const std::shared_ptr<ActionExecutor>& action_executor) {
            WRENCH_INFO("In the lambda execute!!");
            const auto src_location = invocation->_registered_function->_function->_image;
            const auto dst_location = FileLocation::LOCATION(_state_of_the_system->_head_storage_service, src_location->getFile());
            StorageService::copyFile(src_location, dst_location);
        };
        const std::function lambda_terminate = [](const std::shared_ptr<ActionExecutor>& action_executor) {
        };

        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "download_image_" + invocation->_registered_function->_function->_image->getFile()->getID(),
                0, 0, lambda_execute, lambda_terminate));

        // Spin up an ActionExecutor service, and have it send us back a custom message
        auto custom_message = new ServerlessComputeServiceDownloadCompleteMessage(
            action,
            invocation->_registered_function->_function->_image->getFile(), 0);

        auto action_executor = std::make_shared<ActionExecutor>(
            this->getHostname(),
            0,
            0,
            0,
            false,
            this->commport,
            custom_message,
            action,
            nullptr);

        action_executor->setSimulation(this->simulation_);
        WRENCH_INFO("Starting an action executor...");
        action_executor->start(action_executor, true, false); // Daemonized, no auto-restart
    }

    /**
     * @brief
     * 
     */
    void ServerlessComputeService::scheduleInvocations() {
        // Collect all invocations that are now schedulable (i.e. their image is on the head node)
        std::vector<std::shared_ptr<Invocation>> schedulableInvocations;
        while (!_state_of_the_system->_schedulableInvocations.empty()) {
            auto invocation = _state_of_the_system->_schedulableInvocations.front();
            _state_of_the_system->_schedulableInvocations.pop();
            schedulableInvocations.push_back(invocation);
        }
    
        auto imageDecision = _scheduler->manageImages(schedulableInvocations, _state_of_the_system);
    
        // For each compute node, initiate image copy (from head node) for any required images that are missing.
        for (const auto& nodeEntry : imageDecision->imagesToCopy) {
            const std::string& computeHost = nodeEntry.first;
            for (const auto& image : nodeEntry.second) {
                initiateImageCopyToComputeHost(computeHost, image);
            }
        }
    
        // Similarly, trigger removal actions for images that are present but not needed.
        for (const auto& nodeEntry : imageDecision->imagesToRemove) {
            const std::string& computeHost = nodeEntry.first;
            for (const auto& image : nodeEntry.second) {
                initiateImageRemovalFromComputeHost(computeHost, image);
            }
        }
    
        // use the scheduler to assign invocations to compute nodes.
        auto schedulingDecisions = _scheduler->scheduleFunctions(schedulableInvocations, _state_of_the_system);
        for (const auto& decision : schedulingDecisions) {
            auto invocation = decision.first;
            auto target_host = decision.second;
            // Record the scheduling decision.
            _state_of_the_system->_scheduling_decisions[invocation] = target_host;
            // Enqueue the invocation for dispatch.
            _state_of_the_system->_scheduledInvocations.push(invocation);
        }
    }

    void ServerlessComputeService::initiateImageCopyToComputeHost(const std::string& computeHost, std::shared_ptr<DataFile> image) {
        // Add the image to the being_copied_images data structure for this host
        _state_of_the_system->_being_copied_images[computeHost].insert(image);
        
        // Initiate an asynchronous action that copies the image (identified by imageID)
        // from the head node storage service to the compute node's storage service.
        // This might involve creating and starting a dedicated ActionExecutor.
        const std::function lambda_terminate = [](const std::shared_ptr<ActionExecutor>& action_executor) {};

        const std::function lambda_execute = [computeHost, image, this](const std::shared_ptr<ActionExecutor>& action_executor) {
            WRENCH_INFO("In the image copy lambda execute!!");

            // Copy the image file from the head host to the current host's storage service
            auto head_host_image_path = FileLocation::LOCATION(_state_of_the_system->_head_storage_service, image);
            auto local_image_path = wrench::FileLocation::LOCATION(_state_of_the_system->_compute_storages[computeHost], image);
            StorageService::copyFile(head_host_image_path, local_image_path);
            WRENCH_INFO("Done with the lambda execute!!");
        };

        // Create the action and run it in an action executor
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "copy_image_" + image->getID() + "_to_" + computeHost,
                0, 0, lambda_execute, lambda_terminate));

        auto custom_message = new ServerlessComputeServiceNodeCopyCompleteMessage(
            action,
            image, 
            computeHost,
            0);

        auto action_executor = std::make_shared<ActionExecutor>(
            computeHost,
            1,
            0,
            0,
            false,
            this->commport,
            custom_message,
            action,
            nullptr);

        action_executor->setSimulation(this->simulation_);
        WRENCH_INFO("Starting an action executor...");
        action_executor->start(action_executor, true, false);

        WRENCH_INFO("Initiating image copy for image [%s] to compute host [%s]", image->getID().c_str(), computeHost.c_str());
    }

    void ServerlessComputeService::initiateImageRemovalFromComputeHost(const std::string& computeHost, std::shared_ptr<DataFile> image) {
        // Immediately remove the image from the copied_images data structure
        _state_of_the_system->_copied_images[computeHost].erase(image);
        
        // Now remove the file from the storage service
        auto storage_service = _state_of_the_system->_compute_storages[computeHost];
        auto image_location = FileLocation::LOCATION(storage_service, image);
        
        try {
            StorageService::removeFileAtLocation(image_location);
            WRENCH_INFO("Removed image [%s] from compute host [%s]", image->getID().c_str(), computeHost.c_str());
        } catch (const std::exception& e) {
            WRENCH_WARN("Failed to remove image [%s] from compute host [%s]: %s", 
                      image->getID().c_str(), computeHost.c_str(), e.what());
        }
    }

}; // namespace wrench


