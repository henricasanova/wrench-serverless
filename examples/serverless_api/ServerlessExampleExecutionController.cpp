/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller implementation that repeatedly submits two compute actions at a time
 ** to the bare-metal compute service
 **/

#include <iostream>
#include <utility>

#include "ServerlessExampleExecutionController.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000000ULL)

WRENCH_LOG_CATEGORY(custom_controller, "Log category for ServerlessExampleExecutionController");

namespace wrench {

    class MyFunctionInput: public FunctionInput {
    public:
        MyFunctionInput(int x1, int x2) : x1_(x1), x2_(x2) {}

        int x1_;
        int x2_;
    };

    class FCFSServerlessScheduler : public ServerlessScheduler {
        public:
            FCFSServerlessScheduler() = default;
            virtual ~FCFSServerlessScheduler() = default;
        
            // Analyze the list of schedulable invocations and determine per-compute-node image copy/removal decisions.
            virtual ImageManagementDecision manageImages(
                const std::vector<std::shared_ptr<Invocation>>& schedulableInvocations,
                std::shared_ptr<StateOfTheSystem> state
            ) override {
                ImageManagementDecision decision;
                
                // TODO: Implement this.
                // auto availableCores = state->getAvailableCoresMap();
                
                // requiredImageIDs: compute node -> set of required image IDs.
                std::map<std::string, std::unordered_set<std::string>> requiredImageIDs;
                // requiredDataFiles: compute node -> vector of required DataFile pointers.
                std::map<std::string, std::vector<std::shared_ptr<DataFile>>> requiredDataFiles;
                
                // FCFS assignment: for each invocation, assign it to the first compute node with an available core.
                for (const auto& inv : schedulableInvocations) {
                    // Retrieve the DataFile for the image associated with this invocation.
                    auto imageDataFile = inv->getRegisteredFunction()->getFunctionImage();
                    std::string imageID = imageDataFile->getID();
                    bool assigned = false;
                    for (auto& nodeEntry : availableCores) {
                        if (nodeEntry.second > 0) {
                            // Record that this node requires this image.
                            requiredImageIDs[nodeEntry.first].insert(imageID);
                            auto& vec = requiredDataFiles[nodeEntry.first];
                            // Add the DataFile pointer if not already added.
                            bool exists = false;
                            for (const auto& df : vec) {
                                if (df->getID() == imageID) {
                                    exists = true;
                                    break;
                                }
                            }
                            if (!exists) {
                                vec.push_back(imageDataFile);
                            }
                            nodeEntry.second--; // Use one core from this node.
                            assigned = true;
                            break;
                        }
                    }
                    // If no node was available, the invocation is not assigned.
                }
                
                // For each compute node, determine which images are missing (to be copied)
                // and which are extra (to be removed).
                auto computeNodes = state->getComputeNodes();
                for (const auto& node : computeNodes) {
                    std::unordered_set<std::string> reqIDs;
                    if (requiredImageIDs.find(node) != requiredImageIDs.end()) {
                        reqIDs = requiredImageIDs[node];
                    }
                    // Get the current images on the compute node.
                    auto currentDataFiles = state->getImagesOnComputeNode(node);
                    std::unordered_set<std::string> currentIDs;
                    for (const auto& df : currentDataFiles) {
                        currentIDs.insert(df->getID());
                    }
                    
                    // For each required image missing from the node, add it to imagesToCopy.
                    for (const auto& reqID : reqIDs) {
                        if (currentIDs.find(reqID) == currentIDs.end()) {
                            for (const auto& df : requiredDataFiles[node]) {
                                if (df->getID() == reqID) {
                                    decision.imagesToCopy[node].push_back(df);
                                    break;
                                }
                            }
                        }
                    }
                    
                    // For each image present on the node that is not required, add it to imagesToRemove.
                    for (const auto& currID : currentIDs) {
                        if (reqIDs.find(currID) == reqIDs.end()) {
                            for (const auto& df : currentDataFiles) {
                                if (df->getID() == currID) {
                                    decision.imagesToRemove[node].push_back(df);
                                    break;
                                }
                            }
                        }
                    }
                }
                
                return decision;
            }
            
            // Assign invocations to compute nodes in FCFS order.
            virtual std::vector<std::pair<std::shared_ptr<Invocation>, std::string>> scheduleFunctions(
                const std::vector<std::shared_ptr<Invocation>>& schedulableInvocations,
                std::shared_ptr<StateOfTheSystem> state
            ) override {
                std::vector<std::pair<std::shared_ptr<Invocation>, std::string>> schedulingDecisions;
                auto availableCores = state->getAvailableCoresMap();
                // For each invocation, assign it to the first compute node with an available core.
                for (const auto& inv : schedulableInvocations) {
                    bool scheduled = false;
                    for (auto& nodeEntry : availableCores) {
                        if (nodeEntry.second > 0) {
                            schedulingDecisions.push_back({inv, nodeEntry.first});
                            nodeEntry.second--;
                            scheduled = true;
                            break;
                        }
                    }
                    if (!scheduled) {
                        // No available cores remain; break out.
                        break;
                    }
                }
                return schedulingDecisions;
            }
        };        

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param num_actions: the number of actions
     * @param compute_services: a set of compute services available to run actions
     * @param storage_services: a set of storage services available to store data files
     * @param hostname: the name of the host on which to start the WMS
     */
    ServerlessExampleExecutionController::ServerlessExampleExecutionController(std::shared_ptr<ServerlessComputeService> compute_service,
                                                                               std::shared_ptr<SimpleStorageService> storage_service,
                                                                               const std::string &hostname) : ExecutionController(hostname, "me"),
                                                                                                              compute_service(std::move(compute_service)),
                                                                                                              storage_service(std::move(storage_service)) {
    }

    /**
     * @brief main method of the TwoActionsAtATimeExecutionController daemon
     *
     * @return 0 on completion
     *
     */
    int ServerlessExampleExecutionController::main() {
        WRENCH_INFO("ServerlessExampleExecutionController started");

        // TODO: Interact with Serverless provider to do stuff
        // Register a function
        auto function_manager = this->createFunctionManager();
        std::function lambda = [](const std::shared_ptr<FunctionInput>& input, const std::shared_ptr<StorageService>& service) -> std::string {
            auto real_input = std::dynamic_pointer_cast<MyFunctionInput>(input);
            WRENCH_INFO("I AM USER CODE");
            return "Processed: " + std::to_string(real_input->x1_ + real_input->x2_);
        };

        auto image_file = wrench::Simulation::addFile("input_file", 100 * MB);
        auto source_code = wrench::Simulation::addFile("source_code", 10 * MB);
        auto image_location = wrench::FileLocation::LOCATION(this->storage_service, image_file);
        auto code_location = wrench::FileLocation::LOCATION(this->storage_service, source_code);
        wrench::StorageService::createFileAtLocation(image_location);
        wrench::StorageService::createFileAtLocation(code_location);

        auto function1 = function_manager->createFunction("Function 1", lambda, image_location, code_location);

        WRENCH_INFO("Registering function 1");
        function_manager->registerFunction(function1, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);
        WRENCH_INFO("Function 1 registered");

        // Try to register the same function name
        WRENCH_INFO("Trying to register function 1 again");
        try {
            function_manager->registerFunction(function1, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);
        } catch (ExecutionException& expected) {
            WRENCH_INFO("As expected, got exception: %s", expected.getCause()->toString().c_str());

        }

        auto function2 = function_manager->createFunction("Function 2", lambda, image_location, code_location);
        // Try to invoke a function that is not registered yet
        WRENCH_INFO("Invoking a non-registered function");
        auto input = std::make_shared<MyFunctionInput>(1,2);

        try {
            function_manager->invokeFunction(function2, this->compute_service, input);
        } catch (ExecutionException& expected) {
            WRENCH_INFO("As expected, got exception: %s", expected.getCause()->toString().c_str());
        }

        WRENCH_INFO("Registering function 2");
        function_manager->registerFunction(function2, this->compute_service, 10, 2000 * MB, 8000 * MB, 10 * MB, 1 * MB);
        WRENCH_INFO("Function 2 registered");

        std::vector<std::shared_ptr<Invocation>> invocations;

        //TODO: Should the EC be responsible for keeping track of its invocations?
        for (unsigned char i = 0; i < 200; i++) {
            WRENCH_INFO("Invoking function 1");
            invocations.push_back(function_manager->invokeFunction(function1, this->compute_service, input));
            WRENCH_INFO("Function 1 invoked");
            // wrench::Simulation::sleep(1);
        }

        WRENCH_INFO("Waiting for all invocations to complete");
        function_manager->wait_all(invocations);
        WRENCH_INFO("All invocations completed");

        WRENCH_INFO("Invoking function 2");
        std::shared_ptr<Invocation> new_invocation = function_manager->invokeFunction(function2, this->compute_service, input);
        WRENCH_INFO("Function 2 invoked");

        try {
            new_invocation->isSuccess();
        } catch (std::runtime_error& expected) {
            WRENCH_INFO("As expected, got exception");
        }

        try {
            new_invocation->getOutput();
        } catch (std::runtime_error& expected) {
            WRENCH_INFO("As expected, got exception");
        }

        try {
            new_invocation->getFailureCause();
        } catch (std::runtime_error& expected) {
            WRENCH_INFO("As expected, got exception");
        }

        function_manager->wait_one(new_invocation);

        try {
            new_invocation->getOutput();
            WRENCH_INFO("First check passed");
            new_invocation->isSuccess();
            WRENCH_INFO("Second check passed");
            new_invocation->getFailureCause();
            WRENCH_INFO("Third check passed");
        } catch (std::runtime_error& expected) {
            WRENCH_INFO("Not expected, got exception");
        }

        // wrench::Simulation::sleep(100);
        //
        // WRENCH_INFO("Invoking function 1 AGAIN");
        // function_manager->invokeFunction(function1, this->compute_service, input);
        // WRENCH_INFO("Function 1 invoked");

        wrench::Simulation::sleep(1000000);
        // WRENCH_INFO("Execution complete");

        // function_manager->invokeFunction(function2, this->compute_service, input);
        // function_manager->invokeFunction(function1, this->compute_service, input);

        return 0;
    }



}// namespace wrench
