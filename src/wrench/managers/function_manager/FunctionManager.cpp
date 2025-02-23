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
     * @brief 
     * 
     * @param name 
     * @param lambda 
     * @param image 
     * @param code 
     * @return std::shared_ptr<Function> 
     */
    std::shared_ptr<Function> FunctionManager::createFunction(const std::string& name,
                                                                     const std::function<std::string(const std::shared_ptr<FunctionInput>&, const std::shared_ptr<StorageService>&)>& lambda,
                                                                     const std::shared_ptr<FileLocation>& image,
                                                                     const std::shared_ptr<FileLocation>& code) {
                                                                        
        // Create the notion of a function
        return std::make_shared<Function>(name, lambda, image, code);
    }

    /**
     * @brief 
     * 
     * @param function 
     * @param sl_compute_service 
     * @param time_limit_in_seconds 
     * @param disk_space_limit_in_bytes 
     * @param RAM_limit_in_bytes 
     * @param ingress_in_bytes 
     * @param egress_in_bytes 
     * @return true 
     * @return false 
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
    *
    */
    std::shared_ptr<Invocation> FunctionManager::invokeFunction(std::shared_ptr<Function> function,
                                                                const std::shared_ptr<ServerlessComputeService>& sl_compute_service,
                                                                std::shared_ptr<FunctionInput> function_invocation_args) {
        WRENCH_INFO("Function [%s] invoked with compute service [%s]", function->getName().c_str(), sl_compute_service->getName().c_str());
        // Pass in the function manager's commport as the notify commport
        return sl_compute_service->invokeFunction(function, function_invocation_args, this->commport);
    }

    // /**
    // *
    // */
    // FunctionInvocation::is_running() {
    //     // State finding method
    // }

    // /**
    // *
    // */
    // FunctionInvocation::is_done() {
    //     // State finding method
    // }

    // /**
    // *
    // */
    // FunctionOutput FunctionInvocation::get_output() {
    //     // State finding method
    // }

    // /**
    // *
    // */
    // FunctionInvocation::wait_one(one) {

    // }

    // /**
    // *
    // */
    // FunctionInvocation::wait_any(one) {

    // }

    // /**
    // *
    // */
    // FunctionInvocation::wait_all(list) {

    // }

    /**
     * @brief Main method of the daemon that implements the JobManager
     * @return 0 on success
     */
    int FunctionManager::main() {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);
        WRENCH_INFO("New Function Manager starting (%s)", this->commport->get_cname());
        
        while (processNextMessage()) {
            // TODO: Do something
        }

        return 0;
    }

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
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
            processFunctionInvocationComplete();
            return true;
        }
        else if (auto fmfc_msg = std::dynamic_pointer_cast<FunctionManagerFunctionCompletedMessage>(message)) {
            // TODO: Notify some controller?
            return true;
        }
        else if (auto fmff_msg = std::dynamic_pointer_cast<FunctionManagerFunctionFailedMessage>(message)) {
            // TODO: processFunctionInvocationFailure();
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
    void FunctionManager::processFunctionInvocationComplete() {
        WRENCH_INFO("Some Invocation Complete");
    }

    /**
     * @brief TODO
     * 
     */
    void FunctionManager::processFunctionInvocationFailure() {

    }


}// namespace wrench
