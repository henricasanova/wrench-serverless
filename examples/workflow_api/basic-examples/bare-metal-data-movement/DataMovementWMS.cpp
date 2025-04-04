/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** A Workflow Management System (WMS) implementation that operates on a workflow
 ** with a single task that has two input files and two output files as follows:
 ** 
 **  - Copy the first input file from the first storage service to the second one
 **  - Runs the task so that it produces its output files on the second storage service
 **  - Copy the task's first output file to the first storage service
 **  - Delete the task's second output file on the second storage service
 **/

#include <iostream>

#include "DataMovementWMS.h"

WRENCH_LOG_CATEGORY(custom_wms, "Log category for DataMovementWMS");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param workflow: the workflow to execute
     * @param bare_metal_compute_service: a bare-metal compute service available to run tasks
     * @param storage_service1: a storage services available to store files
     * @param storage_service2: a storage services available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    DataMovementWMS::DataMovementWMS(const std::shared_ptr<Workflow> &workflow,
                                     const std::shared_ptr<BareMetalComputeService> &bare_metal_compute_service,
                                     const std::shared_ptr<StorageService> &storage_service1,
                                     const std::shared_ptr<StorageService> &storage_service2,
                                     const std::string &hostname) : ExecutionController(hostname, "data-movement"),
                                                                    workflow(workflow), bare_metal_compute_service(bare_metal_compute_service),
                                                                    storage_service1(storage_service1), storage_service2(storage_service2) {}

    /**
     * @brief main method of the DataMovementWMS daemon
     *
     * @return 0 on completion
     *
     */
    int DataMovementWMS::main() {
        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("WMS starting on host %s", Simulation::getHostName().c_str());
        WRENCH_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Create a data movement manager so that we can create/submit jobs */
        auto data_movement_manager = this->createDataMovementManager();

        /* Get references to the task and files */
        auto task = this->workflow->getTaskByID("task");
        auto infile_1 = wrench::Simulation::getFileByID("infile_1");
        auto infile_2 = wrench::Simulation::getFileByID("infile_2");
        auto outfile_1 = wrench::Simulation::getFileByID("outfile_1");
        auto outfile_2 = wrench::Simulation::getFileByID("outfile_2");

        /* Synchronously copy infile_1 from storage_service1 to storage_service2 */
        WRENCH_INFO("Synchronously copying file infile_1 from storage_service1 to storage_service2");
        data_movement_manager->doSynchronousFileCopy(FileLocation::LOCATION(storage_service1, infile_1),
                                                     FileLocation::LOCATION(storage_service2, infile_1));
        WRENCH_INFO("File copy complete");


        /* Now let's create a map of file locations, stating for each file
         * where it should be read/written while the task executes */
        std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations;

        file_locations[infile_1] = FileLocation::LOCATION(storage_service2, infile_1);
        file_locations[infile_2] = FileLocation::LOCATION(storage_service1, infile_2);
        file_locations[outfile_1] = FileLocation::LOCATION(storage_service2, outfile_1);
        file_locations[outfile_2] = FileLocation::LOCATION(storage_service2, outfile_2);

        /* Create the standard job */
        WRENCH_INFO("Creating a  job to execute task %s", task->getID().c_str());
        auto job = job_manager->createStandardJob(task, file_locations);

        /* Submit the job to the compute service */
        WRENCH_INFO("Submitting job to the compute service");
        job_manager->submitJob(job, bare_metal_compute_service);

        /* Wait for a workflow execution event and process it. In this case we know that
         * the event will be a StandardJobCompletionEvent, which is processed by the method
         * processEventStandardJobCompletion() that this class overrides. */
        WRENCH_INFO("Waiting for next event");
        this->waitForAndProcessNextEvent();

        /* Let's copy outfile_1 from storage_service2 to storage_service1, and let's
         * do it asynchronously for kicks */
        WRENCH_INFO("Asynchronously copying outfile_1 from storage_service2 to storage_service1");
        data_movement_manager->initiateAsynchronousFileCopy(FileLocation::LOCATION(storage_service2, outfile_1),
                                                            FileLocation::LOCATION(storage_service1, outfile_1));

        /* Just for kicks again, let's wait for the next event using  the low-level
         * waitForNextEvent() instead of waitForAndProcessNextEvent() */
        WRENCH_INFO("Waiting for an event");
        try {
            auto event = this->waitForNextEvent();
            // Check that it is the expected event, just in  case
            if (auto file_copy_completion_event = std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(
                        event)) {
                WRENCH_INFO("Notified of the file copy completion for file %s, as expected",
                            file_copy_completion_event->src->getFile()->getID().c_str());
            } else {
                throw std::runtime_error("Unexpected event (" + event->toString() + ")");
            }
        } catch (ExecutionException &e) {
            throw std::runtime_error("Unexpected workflow execution exception (" +
                                     std::string(e.what()) + ")");
        }

        /* Delete outfile_2 on storage_service2 */
        WRENCH_INFO("Deleting file outfile_2 from storage_service2");
        StorageService::deleteFileAtLocation(FileLocation::LOCATION(storage_service2, outfile_2));
        WRENCH_INFO("File deleted");

        WRENCH_INFO("Workflow execution complete");
        return 0;
    }

    /**
     * @brief Process a standard job completion event
     *
     * @param event: the event
     */
    void DataMovementWMS::processEventStandardJobCompletion(const std::shared_ptr<StandardJobCompletedEvent> &event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's first (and in our case only) task */
        auto task = job->getTasks().at(0);
        WRENCH_INFO("Notified that a standard job has completed task %s", task->getID().c_str());
    }

    /**
     * @brief Process a standard job failure event
     *
     * @param event: the event
     */
    void DataMovementWMS::processEventStandardJobFailure(const std::shared_ptr<StandardJobFailedEvent> &event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's first (and in our case only) task */
        auto task = job->getTasks().at(0);
        /* Print some error message */
        WRENCH_INFO("Notified that a standard job has failed for task %s with error %s",
                    task->getID().c_str(),
                    event->failure_cause->toString().c_str());
        throw std::runtime_error("ABORTING DUE TO JOB FAILURE");
    }


}// namespace wrench
