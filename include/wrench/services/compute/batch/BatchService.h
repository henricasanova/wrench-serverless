/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_BATCH_SERVICE_H
#define WRENCH_BATCH_SERVICE_H

#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/standard_job_executor/StandardJobExecutor.h"
#include "wrench/services/compute/batch/BatchJob.h"
#include "wrench/services/compute/batch/BatschedNetworkListener.h"
#include "wrench/services/compute/batch/BatchServiceProperty.h"
#include "wrench/services/helpers/Alarm.h"
#include "wrench/workflow/job/StandardJob.h"
#include "wrench/workflow/job/WorkflowJob.h"
#include <deque>
#include <queue>
#include <set>
#include <tuple>

namespace wrench {

    class WorkloadTraceFileReplayer; // forward

    /**
     * @brief A batch-scheduled compute service that manages a set of compute hosts and
     *        controls access to their resource via a batch queue.
     *
     *        In the current implementation of
     *        this service, like for many of its real-world counterparts, it does not handle memory
     *        partitioning among jobs on the same host. It also does not simulate effects
     *        of memory sharing (e.g., swapping). When multiple jobs share hosts,
     *        which can happen when jobs require only a few cores per host and can thus
     *        be co-located on the same hosts in a non-exclusive fashion, each job simply runs as if it had access to the
     *        full RAM of each compute host it is scheduled on.
     */
    class BatchService : public ComputeService {

        /**
         * @brief A Batch Service
         */
    private:

        std::map<std::string, std::string> default_property_values =
                {{BatchServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,                 "1024"},
                 {BatchServiceProperty::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD,"1024"},
                 {BatchServiceProperty::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, "1024"},
                 {BatchServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,              "1024"},
                 {BatchServiceProperty::THREAD_STARTUP_OVERHEAD,                     "0"},
                 {BatchServiceProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD,           "1024"},
                 {BatchServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, "1024"},
                 {BatchServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                 {BatchServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,    "1024"},
                 {BatchServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,     "1024"},
                 {BatchServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD,         "1024"},
                 {BatchServiceProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD,           "1024"},
                 {BatchServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD,           "1024"},
                 {BatchServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                 {BatchServiceProperty::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD, "1024"},
                 {BatchServiceProperty::HOST_SELECTION_ALGORITHM,                    "FIRSTFIT"},
                #ifdef ENABLE_BATSCHED
                 {BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM,                  "easy_bf"},
                 {BatchServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM,              "fcfs"},
                #else
                        {BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM,            "FCFS"},
                #endif
                 {BatchServiceProperty::BATCH_RJMS_DELAY,                            "0"},
                 {BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE,               ""}
                };

    public:
        BatchService(std::string &hostname,
                     bool supports_standard_jobs,
                     bool supports_pilot_jobs,
                     std::vector<std::string> compute_hosts,
                     std::map<std::string, std::string> plist = {},
                     double scratch_size = 0);

        //returns jobid,started time, running time
//        std::vector<std::tuple<unsigned long, double, double>> getJobsInQueue();


        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/
        std::map<std::string,double> getStartTimeEstimates(std::set<std::tuple<std::string,unsigned int,unsigned int, double>>);

        /***********************/
        /** \endcond          **/
        /***********************/

        ~BatchService() override;


    private:
        friend class WorkloadTraceFileReplayer;

        BatchService(std::string hostname,
                     bool supports_standard_jobs,
                     bool supports_pilot_jobs,
                     std::vector<std::string> compute_hosts,
                     unsigned long cores_per_host,
                     double ram_per_host,
                     std::map<std::string, std::string> plist,
                     std::string suffix,
                     double scratch_size = 0);

        unsigned int batsched_port; // ONLY USED FOR BATSCHED

        //submits a standard job
        void submitStandardJob(StandardJob *job, std::map<std::string, std::string> &batch_job_args) override;

        //submits a standard job
        void submitPilotJob(PilotJob *job, std::map<std::string, std::string> &batch_job_args) override;

        // terminate a standard job
        void terminateStandardJob(StandardJob *job) override;

        // terminate a pilot job
        void terminatePilotJob(PilotJob *job) override;

        std::vector<std::tuple<std::string, double, double, double, double, unsigned int>> workload_trace;
        std::shared_ptr<WorkloadTraceFileReplayer> workload_trace_replayer;

        bool clean_exit = false;

        //Configuration to create randomness in measurement period initially
        unsigned long random_interval = 10;

        //create alarms for standard jobs
        std::map<std::string,std::shared_ptr<Alarm>> standard_job_alarms;

        //alarms for pilot jobs (only one pilot job alarm)
        std::map<std::string,std::shared_ptr<Alarm>> pilot_job_alarms;


        /* Resources information in Batchservice */
        unsigned long total_num_of_nodes;
        unsigned long num_cores_per_node;
        std::map<std::string, unsigned long> nodes_to_cores_map;
        std::vector<double> timeslots;
        std::map<std::string, unsigned long> available_nodes_to_cores;
        std::map<unsigned long, std::string> host_id_to_names;
        std::vector<std::string> compute_hosts;
        /*End Resources information in Batchservice */

        // Vector of standard job executors
        std::set<std::shared_ptr<StandardJobExecutor>> running_standard_job_executors;

        // Vector of standard job executors
        std::set<std::shared_ptr<StandardJobExecutor>> finished_standard_job_executors;

        // Master List of batch jobs
        std::set<std::unique_ptr<BatchJob>>  all_jobs;

        //Queue of pending batch jobs
        std::deque<BatchJob *> pending_jobs;
        //A set of running batch jobs
        std::set<BatchJob *> running_jobs;
        // A set of waiting jobs that have been submitted to batsched, but not scheduled
        std::set<BatchJob *> waiting_jobs;



        //Batch scheduling supported algorithms
#ifdef ENABLE_BATSCHED
        std::set<std::string> scheduling_algorithms = {"easy_bf", "conservative_bf", "easy_bf_plot_liquid_load_horizon",
                                                       "energy_bf", "energy_bf_dicho", "energy_bf_idle_sleeper",
                                                       "energy_bf_monitoring",
                                                       "energy_bf_monitoring_inertial", "energy_bf_subpart_sleeper",
                                                       "filler", "killer", "killer2", "rejecter", "sleeper",
                                                       "submitter", "waiting_time_estimator"
        };

        //Batch queue ordering options
        std::set<std::string> queue_ordering_options = {"fcfs", "lcfs", "desc_bounded_slowdown", "desc_slowdown",
                                                        "asc_size", "desc_size", "asc_walltime", "desc_walltime"

        };
#else
        std::set<std::string> scheduling_algorithms = {"FCFS"
        };

        //Batch queue ordering options
        std::set<std::string> queue_ordering_options = {
        };

#endif



        unsigned long generateUniqueJobId();

        void removeJobFromRunningList(BatchJob *job);

        void freeJobFromJobsList(BatchJob* job);

        int main() override;

        bool processNextMessage();

        void startBackgroundWorkloadProcess();

        void processGetResourceInformation(const std::string &answer_mailbox);

        void processStandardJobCompletion(StandardJobExecutor *executor, StandardJob *job);

        void processStandardJobFailure(StandardJobExecutor *executor,
                                       StandardJob *job,
                                       std::shared_ptr<FailureCause> cause);

        void terminateRunningStandardJob(StandardJob *job);

        std::set<std::tuple<std::string, unsigned long, double>> scheduleOnHosts(std::string host_selection_algorithm,
                                                                                 unsigned long, unsigned long, double);

        BatchJob *pickJobForScheduling(std::string);

        //Terminate the batch service (this is usually for pilot jobs when they act as a batch service)
        void cleanup() override;

        // Terminate currently running pilot jobs
        void terminateRunningPilotJobs();

        //Fail the standard jobs inside the pilot jobs
        void failCurrentStandardJobs(std::shared_ptr<FailureCause> cause);

        //Process the pilot job completion
        void processPilotJobCompletion(PilotJob *job);

        //Process standardjob timeout
        void processStandardJobTimeout(StandardJob *job);

        //process pilot job termination request
        void processPilotJobTerminationRequest(PilotJob *job, std::string answer_mailbox);

        //Process standardjob timeout
        void processPilotJobTimeout(PilotJob *job);

        //free up resources
        void freeUpResources(std::set<std::tuple<std::string, unsigned long, double>> resources);

        //send call back to the pilot job submitters
        void sendPilotJobExpirationNotification(PilotJob *job);

        //send call back to the standard job submitters
        void sendStandardJobFailureNotification(StandardJob *job, std::string job_id);

        // Try to schedule a job
        bool scheduleOneQueuedJob();

        // process a job submission
        void processJobSubmission(BatchJob *job, std::string answer_mailbox);


        //start a job
        void startJob(std::set<std::tuple<std::string, unsigned long, double>>, WorkflowJob *,
                      BatchJob *, unsigned long, double, unsigned long);



        //vector of network listeners (only useful when ENABLE_BATSCHED == on)
        std::vector<std::shared_ptr<BatschedNetworkListener>> network_listeners;

        std::map<std::string,double> getStartTimeEstimatesForFCFS(std::set<std::tuple<std::string,unsigned int,unsigned int, double>>);

#ifdef ENABLE_BATSCHED

        friend class BatschedNetworkListener;

        pid_t pid;

//        bool is_bat_sched_ready;

        void startBatsched();
        void stopBatsched();
//        void setBatschedReady(bool v);
//        bool isBatschedReady();

        std::map<std::string,double> getStartTimeEstimatesFromBatsched(std::set<std::tuple<std::string,unsigned int,unsigned int,double>>);

        void startBatschedNetworkListener();

        void notifyJobEventsToBatSched(std::string job_id, std::string status, std::string job_state,
                                       std::string kill_reason);
        void sendAllQueuedJobsToBatsched();

        //process execute events from batsched
        void processExecuteJobFromBatSched(std::string bat_sched_reply);

#endif // ENABLE_BATSCHED


    };
}


#endif //WRENCH_BATCH_SERVICE_H
