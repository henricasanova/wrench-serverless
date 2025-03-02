/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <memory>
#include <string>
#include <boost/algorithm/string/split.hpp>
#include <utility>

#include "wrench/exceptions/ExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/managers/function_manager/FunctionManager.h"

#include <wrench/services/compute/serverless/ServerlessComputeServiceMessage.h>

#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_CommPort.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/helper_services/action_executor/ActionExecutor.h"
#include "wrench/services/helper_services/action_execution_service/ActionExecutionService.h"
#include "wrench/managers/function_manager/FunctionManagerMessage.h"

WRENCH_LOG_CATEGORY(wrench_core_function_manager, "Log category for Function Manager");

namespace wrench {


    /**
     * @brief Constructor
     *
     * @param hostname: the name of host on which the job manager will run
     * @param creator_commport: the commport of the manager's creator
     */
    FunctionManager::FunctionManager(const std::string& hostname, S4U_CommPort *creator_commport) : Service(hostname, "function_manager") {
        this->creator_commport = creator_commport;
    }

    void FunctionManager::stop() {
        // Implementation of stop logic, e.g., cleanup resources or notify shutdown
        this->Service::stop();
    }

    /**
     * @brief Destructor, which kills the daemon (and clears all the jobs)
     */
    FunctionManager::~FunctionManager() {
    	// Any necessary cleanup (if needed) goes here.
	}

    /**
     * @brief Creates a shared pointer to a Function object and returns it
     * 
     * @param name the name of the function
     * @param lambda 
     * @param image the location of image to execute the function on
     * @param code the location of the code to execute
     * @return std::shared_ptr<Function> a shared pointer to the Function object created
     */
    std::shared_ptr<Function> FunctionManager::createFunction(const std::string& name,
                                                                     const std::function<std::string(const std::shared_ptr<FunctionInput>&, const std::shared_ptr<StorageService>&)>& lambda,
                                                                     const std::shared_ptr<FileLocation>& image,
                                                                     const std::shared_ptr<FileLocation>& code) {
                                                                        
        // Create the notion of a function
        return std::make_shared<Function>(name, lambda, image, code);
    }

    /**
     * @brief Registers a function with the ServerlessComputeService
     * 
     * @param function the function to register
     * @param sl_compute_service the ServerlessComputeService to register the function on
     * @param time_limit_in_seconds the time limit for the function execution
     * @param disk_space_limit_in_bytes the disk space limit for the function
     * @param RAM_limit_in_bytes the RAM limit for the function
     * @param ingress_in_bytes the ingress data limit
     * @param egress_in_bytes the egress data limit
     * @return true if the function was registered successfully
     * @throw ExecutionException if the function registration fails
     */
    bool FunctionManager::registerFunction(const std::shared_ptr<Function> function,
                                           const std::shared_ptr<ServerlessComputeService>& sl_compute_service,
                                           int time_limit_in_seconds,
                                           long disk_space_limit_in_bytes,
                                           long RAM_limit_in_bytes,
                                           long ingress_in_bytes,
                                           long egress_in_bytes) {

        WRENCH_INFO("Function [%s] registered with compute service [%s]", function->getName().c_str(), sl_compute_service->getName().c_str());
        // Logic to register the function with the serverless compute service
        return sl_compute_service->registerFunction(function, time_limit_in_seconds, disk_space_limit_in_bytes, RAM_limit_in_bytes, ingress_in_bytes, egress_in_bytes);
    }

    /**
     * @brief Invokes a function on a ServerlessComputeService
     * 
     * @param function the function to invoke
     * @param sl_compute_service the ServerlessComputeService to invoke the function on
     * @param function_invocation_args arguments to pass to the function
     * @return std::shared_ptr<Invocation> a shared pointer to the Invocation object created by the ServerlessComputeService
     */
    std::shared_ptr<Invocation> FunctionManager::invokeFunction(std::shared_ptr<Function> function,
                                                                const std::shared_ptr<ServerlessComputeService>& sl_compute_service,
                                                                std::shared_ptr<FunctionInput> function_invocation_args) {
        WRENCH_INFO("Function [%s] invoked with compute service [%s]", function->getName().c_str(), sl_compute_service->getName().c_str());
        // Pass in the function manager's commport as the notify commport
        return sl_compute_service->invokeFunction(function, function_invocation_args, this->commport);
    }

    /**
     * @brief State finding method to check if an invocation is done
     * 
     * @param invocation the invocation to check
     * @return true if the invocation is done
     * @return false if the invocation is not done
     */
    bool FunctionManager::isDone(std::shared_ptr<Invocation> invocation) {
        if (_finished_invocations.find(invocation) != _finished_invocations.end()) {
            return true;
        }
        return false;
    }

    /**
     * @brief Waits for a single invocation to finish
     * 
     * @param invocation the invocation to wait for
     */
    void FunctionManager::wait_one(std::shared_ptr<Invocation> invocation) {
        
        WRENCH_INFO("FunctionManager::wait_one(): Waiting for invocation to finish");
        auto answer_commport = S4U_CommPort::getTemporaryCommPort();

        // send a "wait one" message to the FunctionManager's commport
        this->commport->putMessage(
            new FunctionManagerWaitOneMessage(
                answer_commport, 
                invocation
            ));

        // unblock up the EC with a wakeup message
        auto msg = answer_commport->getMessage<FunctionManagerWakeupMessage>(
            // this->network_timeout, // commented out for unlimited timeout time
            "FunctionManager::wait_one(): Received an");

        WRENCH_INFO("FunctionManager::wait_one(): Received a wakeup message");
    }

    /**
     * @brief Waits for a list of invocations to finish
     * 
     */
    void FunctionManager::wait_all(std::vector<std::shared_ptr<Invocation>> invocations) {
        
        WRENCH_INFO("FunctionManager::wait_all(): Waiting for list of invocations to finish");
        auto answer_commport = S4U_CommPort::getTemporaryCommPort();

        // send a "wait one" message to the FunctionManager's commport
        this->commport->putMessage(
            new FunctionManagerWaitAllMessage(
                answer_commport, 
                invocations
            ));

        // unblock the EC with a wakeup message
        auto msg = answer_commport->getMessage<FunctionManagerWakeupMessage>(
            // this->network_timeout, // commented out for unlimited timeout time
            "FunctionManager::wait_one(): Received an");
        
        WRENCH_INFO("FunctionManager::wait_all(): Received a wakeup message");
    }

    // /**
    // *
    // */
    // FunctionManager::wait_any(one) {

    // }

    /**
     * @brief Main method of the daemon that implements the FunctionManager
     * @return 0 on success
     */
    int FunctionManager::main() {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);
        WRENCH_INFO("New Function Manager starting (%s)", this->commport->get_cname());
        
        while (processNextMessage()) {
            // TODO: Do something
            processInvocationsBeingWaitedFor();
        }

        return 0;
    }

    /**
     * @brief Processes the next message in the commport
     * 
     * @return true when the FunctionManager daemon should continue processing messages
     * @return false when the FunctionManager daemon should die
     */
    bool FunctionManager::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        } catch (ExecutionException &e) {
            WRENCH_INFO(
                    "Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());
        //        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (std::dynamic_pointer_cast<FunctionManagerWakeupMessage>(message)) {
            // wake up!!
            return true;
        }
        else if (std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // TODO: Die...
            return false;
        }
        else if (auto scsfic_msg = std::dynamic_pointer_cast<ServerlessComputeServiceFunctionInvocationCompleteMessage>(message)) {
            processFunctionInvocationComplete(scsfic_msg->invocation, scsfic_msg->success, scsfic_msg->failure_cause);
            return true;
        }
        else if (auto fmfc_msg = std::dynamic_pointer_cast<FunctionManagerFunctionCompletedMessage>(message)) {
            // TODO: Notify some controller?
            return true;
        }
        else if (auto wait_one_msg = std::dynamic_pointer_cast<FunctionManagerWaitOneMessage>(message)) {
            processWaitOne(wait_one_msg->invocation, wait_one_msg->answer_commport);
            return true;
        }
        else if (auto wait_many_msg = std::dynamic_pointer_cast<FunctionManagerWaitAllMessage>(message)) {
            processWaitAll(wait_many_msg->invocations, wait_many_msg->answer_commport);
            return true;
        }
        else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief TODO
     * 
     */
    void FunctionManager::processFunctionInvocationComplete(std::shared_ptr<Invocation> invocation, 
                                                            bool success, 
                                                            std::shared_ptr<FailureCause> failure_cause) {
        WRENCH_INFO("Some Invocation Complete");
        invocation->_done = true;
        invocation->_success = success;
        invocation->_failure_cause = failure_cause;
        // _pending_invocations.erase(invocation);
        _finished_invocations.insert(invocation);
    }

    /**
     * @brief Processes a "wait one" message
     * 
     * @param invocation the invocation being waited for
     * @param answer_commport the answer commport to send the wakeup message to when the invocation is finished
     */
    void FunctionManager::processWaitOne(std::shared_ptr<Invocation> invocation, S4U_CommPort* answer_commport) {
        WRENCH_INFO("Processing a wait_one message");
        _invocations_being_waited_for.push_back(std::make_pair(invocation, answer_commport));
    }

    /**
     * @brief Processes a "wait many" message
     * 
     * @param invocations the invocations being waited for
     * @param answer_commport the answer commport to send the wakeup message to when the invocations are finished
     */
    void FunctionManager::processWaitAll(std::vector<std::shared_ptr<Invocation>> invocations, S4U_CommPort* answer_commport) {
        WRENCH_INFO("Processing a wait_many message");
        for (auto invocation : invocations) {
            _invocations_being_waited_for.push_back(std::make_pair(invocation, answer_commport));
        }
    }

    /**
     * @brief Iterates through the list of invocations being waited for and checks if they are finished
     * 
     * TODO: There has to be a better way to do this than storing the answer commport in every single invocation LOL
     */
    void FunctionManager::processInvocationsBeingWaitedFor() {
        WRENCH_INFO("Processing invocations being waited for");
        // iterate through the list of invocations being waited for
        if (_invocations_being_waited_for.empty()) {
            return;
        }
        auto it = _invocations_being_waited_for.begin();
        while (it != _invocations_being_waited_for.end()) {
            // check if the invocation is finished
            if (_finished_invocations.find(it->first) != _finished_invocations.end()) {
                // if there's only 1 invocation being waited for remaining, send a wakeup message
                if (_invocations_being_waited_for.size() <= 1) {
                    it->second->putMessage(new FunctionManagerWakeupMessage());
                }
                it = _invocations_being_waited_for.erase(it);
            } else {
                it++;
            }
        }
    }

}// namespace wrench
