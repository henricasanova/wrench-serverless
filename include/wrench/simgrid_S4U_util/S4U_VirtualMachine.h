/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_S4U_VIRTUALMACHINE_H
#define WRENCH_S4U_VIRTUALMACHINE_H

#include <simgrid/s4u/VirtualMachine.hpp>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A wrapper for the simgrid::s4u::VirtualMachine class
     */
    class S4U_VirtualMachine {

    public:

        /** @brief VM state enum */
        enum State {
            DOWN,
            RUNNING,
            SUSPENDED
        };

        S4U_VirtualMachine(const std::string &vm_hostname,
                           unsigned long num_cores,
                           double ram_memory);

        void start(std::string &pm_name);

        void suspend();

        void resume();

        void shutdown();

        void migrate(const std::string &dst_pm_name);


        std::string getPhysicalHostname();
        unsigned long getNumCores();
        double getMemory();
        State getState();
        std::string getStateAsString();


    private:
        State state;
        std::string vm_name;
        simgrid::s4u::VirtualMachine *vm;
        unsigned long num_cores;
        double ram_memory;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_S4U_VIRTUALMACHINE_H
