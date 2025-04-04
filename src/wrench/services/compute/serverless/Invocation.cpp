/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/serverless/Invocation.h"

namespace wrench {

    /**
     * @brief Constructor for Invocation.
     * @param registered_function The registered function to be invoked.
     * @param function_input The input for the function.
     * @param notify_commport The communication port for notifications.
     */
    Invocation::Invocation(std::shared_ptr<RegisteredFunction> registered_function,
                           std::shared_ptr<FunctionInput> function_input,
                           S4U_CommPort* notify_commport) : _registered_function(registered_function),
                                                            _function_input(function_input),
                                                            _done(false),
                                                            _notify_commport(notify_commport)
    {
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
     * @brief Checks if the invocation is done.
     * @return True if the invocation is done, false otherwise.
     */
    bool Invocation::isDone() const { return _done; }

    /**
     * @brief Checks if the invocation was successful.
     * @return True if the invocation was successful, false otherwise.
     */
    bool Invocation::isSuccess() const { 
        if (_done) {
            return _success;
        }
        throw std::runtime_error("Invocation::isSuccess(): Invocation is not done yet");
    }

    /**
     * @brief Gets the registered function.
     * @return A shared pointer to the registered function.
     */
    std::shared_ptr<RegisteredFunction> Invocation::getRegisteredFunction() const { return _registered_function; }

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
