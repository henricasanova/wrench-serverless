/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** TODO: This simulator simulates the execution of a bag-of-actions application, that is, a bunch
 ** of independent compute actions, each with its own input file and its own output file. Actions can be
 ** executed completely independently:
 **
 **   InputFile #0 -> Action #0 -> OutputFile #1
 **   ...
 **   InputFile #n -> Action #n -> OutputFile #n
 **
 ** The compute platform comprises two hosts, ControllerHost and ComputeHost. On ControllerHost runs a simple storage
 ** service and an execution controller (defined in class TwoActionsAtATimeExecutionController). On ComputeHost runs a bare metal
 ** compute service, that has access to the 10 cores of that host.
 **
 ** Example invocation of the simulator for a 10-compute-action workload, with no logging:
 **    ./wrench-example-bare-metal-bag-of-actions 10 ./two_hosts.xml
 **
 ** Example invocation of the simulator for a 10-compute-action workload, with only execution controller logging:
 **    ./wrench-example-bare-metal-bag-of-actions 10 ./two_hosts.xml --log=custom_controller.threshold=info
 **
 ** Example invocation of the simulator for a 6-compute-action workload with full logging:
 **    ./wrench-example-bare-metal-bag-of-actions 6 ./two_hosts.xml --wrench-full-log
 **/


#include <iostream>
#include <wrench.h>

#include "ServerlessExampleExecutionController.h"

/**
 * @brief The Simulator's main function
 *
 * @param argc: argument count
 * @param argv: argument array
 * @return 0 on success, non-zero otherwise
 */
int main(int argc, char **argv) {

    /*
     * Create a WRENCH simulation object
     */
    auto simulation = wrench::Simulation::createSimulation();

    /* Initialize the simulation, which may entail extracting WRENCH-specific and
     * Simgrid-specific command-line arguments that can modify general simulation behavior.
     * Two special command-line arguments are --help-wrench and --help-simgrid, which print
     * details about available command-line arguments. */
    simulation->init(&argc, argv);

    /* Parsing of the command-line arguments for this WRENCH simulation */
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <xml platform file> [--log=custom_controller.threshold=info]" << std::endl;
        exit(1);
    }

    /* Reading and parsing the platform description file, written in XML following the SimGrid-defined DTD,
     * to instantiate the simulated platform */
    std::cerr << "Instantiating simulated platform..." << std::endl;
    simulation->instantiatePlatform(argv[1]);


    /* Instantiate a storage service, and add it to the simulation.
     * A wrench::StorageService is an abstraction of a service on
     * which files can be written and read.  This particular storage service, which is an instance
     * of wrench::SimpleStorageService, is started on UserHost in the
     * platform , which has an attached disk mounted at "/". The SimpleStorageService
     * is a basic storage service implementation provided by WRENCH.
     * Throughout the simulation execution, data files will be located
     * in this storage service, and accessed remotely by the compute service. Note that the
     * storage service is configured to use a buffer size of 50MB when transferring data over
     * the network (i.e., to pipeline disk reads/writes and network revs/sends). */
    std::cerr << "Instantiating a SimpleStorageService on UserHost..." << std::endl;
    auto storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
            "UserHost", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "50MB"}}, {}));

    /* Instantiate a serverless compute service */
    std::cerr << "Instantiating a serverless compute service on ServerlessHeadNode..." << std::endl;
    std::vector<std::string> batch_nodes = {"ServerlessComputeNode1", "ServerlessComputeNode2"};
    auto serverless_provider = simulation->add(new wrench::ServerlessComputeService(
            "ServerlessHeadNode", batch_nodes, "/", {}, {}));

    /* Instantiate an Execution controller, to be stated on UserHost, which is responsible
     * for executing the workflow-> */
    auto wms = simulation->add(
            new wrench::ServerlessExampleExecutionController(serverless_provider, storage_service, "UserHost"));

    /* Launch the simulation. This call only returns when the simulation is complete. */
    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation->launch();
    } catch (std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cerr << "Simulation done!" << std::endl;

    return 0;
}
