/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_PILOTJOBMANAGER_H
#define WRENCH_PILOTJOBMANAGER_H

#include <vector>
#include <set>

#include "wrench/services/Service.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"


namespace wrench {

    class FailureCause;

    class WMS;

    class ExecutionController;

    class Workflow;

    class WorkflowTask;

    class WorkflowFile;

    class Job;

    class PilotJob;

    class CompoundJob;

    class StandardJob;

    class ComputeService;

    class StorageService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/


    /**
 * @brief A helper daemon (co-located with and explicitly started by an execution controller), which is used to
 *        handle all job executions
 */
    class JobManager : public Service {

    public:

        void stop() override;

        void kill();

        std::shared_ptr<CompoundJob> createCompoundJob(std::string name);

        std::shared_ptr<StandardJob> createStandardJob(std::vector<WorkflowTask *> tasks,
                                                       const std::map<WorkflowFile *, std::shared_ptr<FileLocation> >& file_locations,
                                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> pre_file_copies,
                                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> post_file_copies,
                                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> cleanup_file_deletions);

        std::shared_ptr<StandardJob> createStandardJob(std::vector<WorkflowTask *> tasks,
                                                       std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> file_locations,
                                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> pre_file_copies,
                                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> post_file_copies,
                                                       std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> cleanup_file_deletions);


        std::shared_ptr<StandardJob> createStandardJob(std::vector<WorkflowTask *> tasks,
                                                       std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations);

        std::shared_ptr<StandardJob> createStandardJob(std::vector<WorkflowTask *> tasks,
                                                       std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> file_locations);

        std::shared_ptr<StandardJob> createStandardJob(WorkflowTask *task,
                                                       std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations);

        std::shared_ptr<StandardJob> createStandardJob(WorkflowTask *task,
                                                       std::map<WorkflowFile *, std::vector<std::shared_ptr<FileLocation>>> file_locations);

        std::shared_ptr<StandardJob> createStandardJob(std::vector<WorkflowTask *> tasks);

        std::shared_ptr<StandardJob> createStandardJob(WorkflowTask *task);

        std::shared_ptr<PilotJob> createPilotJob();

        void submitJob(std::shared_ptr<StandardJob> job, std::shared_ptr<ComputeService> compute_service,
                       std::map<std::string, std::string> service_specific_args = {});

        void submitJob(std::shared_ptr<CompoundJob> job, std::shared_ptr<ComputeService> compute_service,
                       std::map<std::string, std::string> service_specific_args = {});

        void submitJob(std::shared_ptr<PilotJob> job, std::shared_ptr<ComputeService> compute_service,
                       std::map<std::string, std::string> service_specific_args = {});

        void terminateJob(std::shared_ptr<StandardJob> job);

        void terminateJob(std::shared_ptr<CompoundJob> job);

        void terminateJob(std::shared_ptr<PilotJob> job);

        std::string &getCreatorMailbox();

        unsigned long getNumRunningPilotJobs();

        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        ~JobManager() override;

    protected:

        friend class ExecutionController;
        friend class WMS;

        explicit JobManager(std::string hostname, std::string &creator_mailbox);

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        int main() override;

        void dispatchJobs();

        void dispatchJob(std::shared_ptr<CompoundJob> job);

        bool processNextMessage();

        void
        processStandardJobCompletion(std::shared_ptr<StandardJob> job, std::shared_ptr<ComputeService> compute_service);

        void
        processStandardJobFailure(std::shared_ptr<StandardJob> job, std::shared_ptr<ComputeService> compute_service);

        void
        processCompoundJobCompletion(std::shared_ptr<CompoundJob> job, std::shared_ptr<ComputeService> compute_service);

        void
        processCompoundJobFailure(std::shared_ptr<CompoundJob> job, std::shared_ptr<ComputeService> compute_service);

        void processPilotJobStart(std::shared_ptr<PilotJob> job, std::shared_ptr<ComputeService> compute_service);

        void processPilotJobExpiration(std::shared_ptr<PilotJob> job, std::shared_ptr<ComputeService> compute_service);

        void processPilotJobFailure(std::shared_ptr<PilotJob> job, std::shared_ptr<ComputeService> compute_service, std::shared_ptr<FailureCause> cause);

        // Mailbox of the creator of this job manager
        std::string creator_mailbox;

        std::vector<std::shared_ptr<CompoundJob>> jobs_to_dispatch;
        std::set<std::shared_ptr<CompoundJob>> jobs_dispatched;

        unsigned long num_running_pilot_jobs = 0;

        std::map<std::shared_ptr<CompoundJob>, std::map<std::string, std::string>> cjob_args;

        std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<StandardJob>> cjob_to_sjob_map;
        std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<PilotJob>> cjob_to_pjob_map;

        };

    /***********************/
    /** \endcond            */
    /***********************/

};

#endif //WRENCH_PILOTJOBMANAGER_H
