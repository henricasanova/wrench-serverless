/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_SIMULATION_H
#define WRENCH_SIMULATION_H

#include <string>
#include <vector>

#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/services/network_proximity/NetworkProximityService.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/SimulationOutput.h"
#include "wrench/simulation/Terminator.h"
#include "wrench/wms/WMS.h"
#include "wrench/workflow/job/StandardJob.h"


namespace wrench {

    class StorageService;

    /**
     * @brief The simulation state
     */
    class Simulation {

    public:
        Simulation();

        ~Simulation();

        void init(int *, char **);

        void instantiatePlatform(std::string);

        std::vector<std::string> getHostnameList();

        bool hostExists(std::string hostname);

        void launch();

        ComputeService *add(std::unique_ptr<ComputeService> executor);

        StorageService *add(std::unique_ptr<StorageService> executor);

        NetworkProximityService *add(std::unique_ptr<NetworkProximityService> executor);
        WMS *add(std::unique_ptr<WMS>);

        void setFileRegistryService(std::unique_ptr<FileRegistryService> file_registry_service);

//        void setNetworkProximityService(std::unique_ptr<NetworkProximityService> network_proximity_service);

        void stageFile(WorkflowFile *file, StorageService *storage_service);

        void stageFiles(std::map<std::string, WorkflowFile *> files, StorageService *storage_service);

        /** @brief The simulation post-mortem output */
        SimulationOutput output;

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        template<class T>
        void newTimestamp(SimulationTimestamp<T> *event);

        FileRegistryService *getFileRegistryService();

        void shutdownAllNetworkProximityServices();

        std::set<ComputeService *> getRunningComputeServices();
        double getCurrentSimulatedDate();

        static double getHostMemoryCapacity(std::string hostname);

        static unsigned long getHostNumCores(std::string hostname);

        std::set<NetworkProximityService *> getRunningNetworkProximityServices();

        static double getHostFlopRate(std::string hostname);

        static double getMemoryCapacity();

        static void sleep(double duration);

        Terminator* getTerminator();

        /***********************/
        /** \endcond            */
        /***********************/

        /***********************/
        /** \cond INTERNAL     */
        /***********************/


        /***********************/
        /** \endcond           */
        /***********************/

    private:

        std::unique_ptr<S4U_Simulation> s4u_simulation;

        std::unique_ptr<Terminator> terminator;

        std::set<std::unique_ptr<WMS>> wmses;

        std::unique_ptr<FileRegistryService> file_registry_service = nullptr;

        std::set<std::unique_ptr<NetworkProximityService>> network_proximity_services;

        std::set<std::unique_ptr<ComputeService>> compute_services;

        std::set<std::unique_ptr<StorageService>> storage_services;

        void check_simulation_setup();

        void start_all_processes();

    };

};

#endif //WRENCH_SIMULATION_H
