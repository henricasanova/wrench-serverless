/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/serverless/Invocation.h"

#include "wrench/logging/TerminalOutput.h"
WRENCH_LOG_CATEGORY(Invocations, "Log category for Serverless invocations");


namespace wrench {

    /**
     * @brief Constructor
     * @param registered_function The registered function to be invoked
     * @param function_input The input for the function
     * @param notify_commport The commport to notify upon completion/failure
     */
    Invocation::Invocation(const std::shared_ptr<RegisteredFunction> &registered_function,
                           const std::shared_ptr<FunctionInput> &function_input,
                           S4U_CommPort* notify_commport) : _registered_function(registered_function),
                                                            _function_input(function_input),
                                                            _done(false),
                                                            _success(false),
                                                            _notify_commport(notify_commport)
    {
        // WRENCH_INFO("Invocation created for function %s", _registered_function->getFunction()->getName().c_str());
    }

    /**
     * @brief Gets the output of the function invocation.
     * @return A shared pointer to the function output.
     */
    std::shared_ptr<FunctionOutput> Invocation::getOutput() const { 
        if (_done) {
            return _function_output; 
        }
        throw std::runtime_error("Invocation::get_output(): Invocation is not done yet");
    }

    /**
     * @brief Get the invocation's submit date
     * @return A simulated date (or -1.0 if not submitted)
     */
    double Invocation::getSubmitDate() const {
        return _submit_date;
    }

    /**
    * @brief Get the invocation's start date
    * @return A simulated date (or -1.0 if not submitted)
    */
    double Invocation::getStartDate() const {
        return _start_date;
    }

    /**
    * @brief Get the invocation's end date
    * @return A simulated date (or -1.0 if not submitted)
    */
    double Invocation::getEndDate() const {
        return _end_date;
    }

    /**
     * @brief Checks if the invocation is done.
     * @return True if the invocation is done, false otherwise.
     */
    bool Invocation::isDone() const {
        return _done;
    }

    /**
     * @brief Checks if the invocation was successful.
     * @return True if the invocation was successful, false otherwise.
     */
    bool Invocation::hasSucceeded() const { 
        if (_done) {
            return _success;
        }
        throw std::runtime_error("Invocation::isSuccess(): Invocation is not done yet");
    }

    /**
     * @brief Gets the registered function.
     * @return A shared pointer to the registered function.
     */
    std::shared_ptr<RegisteredFunction> Invocation::getRegisteredFunction() const {
        return _registered_function;
    }

    /**
     * @brief Gets the cause of failure.
     * @return A shared pointer to the failure cause.
     */
    std::shared_ptr<FailureCause> Invocation::getFailureCause() const {
        if (_done) {
            return _failure_cause;
        }
        throw std::runtime_error("Invocation::getFailureCause(): Invocation is not done yet or was successful");
    }

} // namespace wrench
