/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>
#include <wrench/services/helpers/ServiceFailureDetectorMessage.h>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"
#include "test_util/HostSwitch.h"
#include "wrench/services/helpers/ServiceFailureDetector.h"
#include "test_util/SleeperVictim.h"
#include "test_util/HostRandomRepeatSwitch.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(standard_job_executor_simulated_failures_test, "Log category for StandardJobExecutorSimulatedFailuresTests");


class StandardJobExecutorSimulatedFailuresTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;

    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;
    wrench::StorageService *storage_service = nullptr;
    wrench::ComputeService *compute_service = nullptr;

    void do_OneFailureCausingWorkUnitRestartOnAnotherHost_test();
//    void do_OneFailureCausingWorkUnitRestartOnSameHost_test();
//    void do_RandomFailures_test();

protected:

    StandardJobExecutorSimulatedFailuresTest() {
        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create two files
        input_file = workflow->addFile("input_file", 10000.0);
        output_file = workflow->addFile("output_file", 20000.0);

        // Create one task
        task = workflow->addTask("task", 3600, 1, 1, 1.0, 0);
        task->addInputFile(input_file);
        task->addOutputFile(output_file);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"FailedHost1\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"FailedHost2\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"StableHost\" speed=\"1f\" core=\"1\"/> "
                          "       <link id=\"link1\" bandwidth=\"100kBps\" latency=\"0\"/>"
                          "       <route src=\"FailedHost1\" dst=\"StableHost\">"
                          "           <link_ctn id=\"link1\"/>"
                          "       </route>"
                          "       <route src=\"FailedHost2\" dst=\"StableHost\">"
                          "           <link_ctn id=\"link1\"/>"
                          "       </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};



#if 0

/**********************************************************************/
/**                FAIL OVER TO SECOND HOST  TEST                    **/
/**********************************************************************/

class OneFailureCausingWorkUnitRestartOnAnotherHostTestWMS : public wrench::WMS {

public:
    OneFailureCausingWorkUnitRestartOnAnotherHostTestWMS(StandardJobExecutorSimulatedFailuresTest *test,
                                                         std::string &hostname, wrench::StorageService *ss) :
            wrench::WMS(nullptr, nullptr, {}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    StandardJobExecutorSimulatedFailuresTest *test;

    int main() override {

        // Starting a FailedHost1 murderer!!
        auto murderer = std::shared_ptr<wrench::HostSwitch>(new wrench::HostSwitch("StableHost", 100, "FailedHost1", wrench::HostSwitch::Action::TURN_OFF));
        murderer->simulation = this->simulation;
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        // Starting a FailedHost1 resurector!!
        auto resurector = std::shared_ptr<wrench::HostSwitch>(new wrench::HostSwitch("StableHost", 1000, "FailedHost1", wrench::HostSwitch::Action::TURN_ON));
        resurector->simulation = this->simulation;
        resurector->start(resurector, true, false); // Daemonized, no auto-restart


        // Create a job manager
        std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
        auto job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service2}
                },
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        this->getWorkflow()->getFileByID("output_file"), this->test->storage_service2,
                        this->test->storage_service1)},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->getWorkflow()->getFileByID("output_file"),
                                                                              this->test->storage_service2)});

        // Create a StandardJobExecutor
        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            {"FailedHost1", "FailedHost2"},
                            job,
                            {std::make_pair("Host1", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}
                    ));
        } catch (std::invalid_argument &e) {
            throw std::runtime_error("Should be able to create a valid standard job executor!");
        }

        executor->start(executor, true, false); // Daemonized, no auto-restart

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        job_manager->forgetJob(job);
        // Submit the standard job to the compute service, making it sure it runs on FailedHost1
        std::map<std::string, std::string> service_specific_args;
        service_specific_args["task"] = "FailedHost1";
        job_manager->submitJob(job, this->test->compute_service, service_specific_args);

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event = this->getWorkflow()->waitForNextExecutionEvent();
        if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION) {
            throw std::runtime_error("Unexpected workflow execution event!");
        }

        // Paranoid check
        if (!this->test->storage_service->lookupFile(this->test->output_file, nullptr)) {
            throw std::runtime_error("Output file not written to storage service");
        }



        return 0;
    }
};

#if ((SIMGRID_VERSION_MAJOR == 3) && (SIMGRID_VERSION_MINOR == 22)) || ((SIMGRID_VERSION_MAJOR == 3) && (SIMGRID_VERSION_MINOR == 21) && (SIMGRID_VERSION_PATCH > 0))
TEST_F(StandardJobExecutorSimulatedFailuresTest, OneFailureCausingWorkUnitRestartOnAnotherHost) {
#else
    TEST_F(StandardJobExecutorSimulatedFailuresTest, DISABLED_OneFailureCausingWorkUnitRestartOnAnotherHost) {
#endif
    DO_TEST_WITH_FORK(do_OneFailureCausingWorkUnitRestartOnAnotherHost_test);
}

void StandardJobExecutorSimulatedFailuresTest::do_OneFailureCausingWorkUnitRestartOnAnotherHost_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";


    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, 10000000000000.0));

    // Create a WMS
    wrench::WMS *wms = nullptr;
    wms = simulation->add(new OneFailureCausingWorkUnitRestartOnAnotherHostTestWMS(this, stable_host, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(stable_host)));
    ASSERT_NO_THROW(simulation->stageFiles({{input_file->getID(), input_file}}, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}

#endif

#if 0

/**********************************************************************/
/**                    RESTART ON SAME HOST                          **/
/**********************************************************************/

class OneFailureCausingWorkUnitRestartOnSameHostTestWMS : public wrench::WMS {

public:
    OneFailureCausingWorkUnitRestartOnSameHostTestWMS(StandardJobExecutorSimulatedFailuresTest *test,
                                                      std::string &hostname, wrench::ComputeService *cs, wrench::StorageService *ss) :
            wrench::WMS(nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    StandardJobExecutorSimulatedFailuresTest *test;

    int main() override {

        // Starting a FailedHost1 murderer!!
        auto murderer = std::shared_ptr<wrench::HostSwitch>(new wrench::HostSwitch("StableHost", 100, "FailedHost1", wrench::HostSwitch::Action::TURN_OFF));
        murderer->simulation = this->simulation;
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        // Starting a FailedHost1 resurector!!
        auto resurector = std::shared_ptr<wrench::HostSwitch>(new wrench::HostSwitch("StableHost", 1000, "FailedHost1", wrench::HostSwitch::Action::TURN_ON));
        resurector->simulation = this->simulation;
        resurector->start(resurector, true, false); // Daemonized, no auto-restart


        // Create a job manager
        std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

        // Create a standard job
        auto job = job_manager->createStandardJob(this->test->task, {{this->test->input_file, this->test->storage_service},
                                                                     {this->test->output_file, this->test->storage_service}});

        // Submit the standard job to the compute service, making it sure it runs on FailedHost1
        job_manager->submitJob(job, this->test->compute_service, {});

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event = this->getWorkflow()->waitForNextExecutionEvent();
        if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION) {
            throw std::runtime_error("Unexpected workflow execution event!");
        }

        // Paranoid check
        if (!this->test->storage_service->lookupFile(this->test->output_file, nullptr)) {
            throw std::runtime_error("Output file not written to storage service");
        }



        return 0;
    }
};

#if ((SIMGRID_VERSION_MAJOR == 3) && (SIMGRID_VERSION_MINOR == 22)) || ((SIMGRID_VERSION_MAJOR == 3) && (SIMGRID_VERSION_MINOR == 21) && (SIMGRID_VERSION_PATCH > 0))
TEST_F(StandardJobExecutorSimulatedFailuresTest, OneFailureCausingWorkUnitRestartOnSameHost) {
#else
    TEST_F(StandardJobExecutorSimulatedFailuresTest, DISABLED_OneFailureCausingWorkUnitRestartOnSameHost) {
#endif
    DO_TEST_WITH_FORK(do_OneFailureCausingWorkUnitRestartOnSameHost_test);
}

void StandardJobExecutorSimulatedFailuresTest::do_OneFailureCausingWorkUnitRestartOnSameHost_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";

    // Create a Compute Service that has access to two hosts
    compute_service = simulation->add(
            new wrench::StandardJobExecutor(stable_host,
                                                (std::map<std::string, std::tuple<unsigned long, double>>){
                                                        std::make_pair("FailedHost1", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
                                                },
                                                100.0,
                                                {}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, 10000000000000.0));

    // Create a WMS
    wrench::WMS *wms = nullptr;
    wms = simulation->add(new OneFailureCausingWorkUnitRestartOnSameHostTestWMS(this, stable_host, compute_service, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(stable_host)));
    ASSERT_NO_THROW(simulation->stageFiles({{input_file->getID(), input_file}}, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}


/**********************************************************************/
/**                    RANDOM FAILURES                               **/
/**********************************************************************/

class RandomFailuresTestWMS : public wrench::WMS {

public:
    RandomFailuresTestWMS(StandardJobExecutorSimulatedFailuresTest *test,
                          std::string &hostname, wrench::ComputeService *cs, wrench::StorageService *ss) :
            wrench::WMS(nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    StandardJobExecutorSimulatedFailuresTest *test;

    int main() override {

        // Create a job manager
        std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

        unsigned long NUM_TRIALS = 1;

        for (unsigned long trial=0; trial < NUM_TRIALS; trial++) {

            WRENCH_INFO("*** Trial %ld", trial);

            // Starting a FailedHost1 random repeat switch!!
            unsigned long seed1 = trial * 2 + 37;
            auto switch1 = std::shared_ptr<wrench::HostRandomRepeatSwitch>(
                    new wrench::HostRandomRepeatSwitch("StableHost", seed1, 10, 100, "FailedHost1"));
            switch1->simulation = this->simulation;
            switch1->start(switch1, true, false); // Daemonized, no auto-restart

            // Starting a FailedHost2 random repeat switch!!
            unsigned long seed2 = trial * 7 + 417;
            auto switch2 = std::shared_ptr<wrench::HostRandomRepeatSwitch>(
                    new wrench::HostRandomRepeatSwitch("StableHost", seed2, 10, 100, "FailedHost2"));
            switch2->simulation = this->simulation;
            switch2->start(switch2, true, false); // Daemonized, no auto-restart

            // Add a task to the workflow
            auto task = this->test->workflow->addTask("task_" + std::to_string(trial), 80, 1, 1, 1.0, 0);
            task->addInputFile(this->test->input_file);
            task->addOutputFile(this->test->output_file);

            // Create a standard job
            auto job = job_manager->createStandardJob(task, {{this->test->input_file, this->test->storage_service},
                                                             {this->test->output_file, this->test->storage_service}});

            // Submit the standard job to the compute service, making it sure it runs on FailedHost1
            job_manager->submitJob(job, this->test->compute_service, {});

            // Wait for a workflow execution event
            std::unique_ptr<wrench::WorkflowExecutionEvent> event = this->getWorkflow()->waitForNextExecutionEvent();
            if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION) {
                throw std::runtime_error("Unexpected workflow execution event!");
            }

            switch1->kill();
            switch2->kill();

            wrench::Simulation::sleep(10.0);
            this->test->workflow->removeTask(task);

        }

        return 0;
    }
};

#if ((SIMGRID_VERSION_MAJOR == 3) && (SIMGRID_VERSION_MINOR == 22)) || ((SIMGRID_VERSION_MAJOR == 3) && (SIMGRID_VERSION_MINOR == 21) && (SIMGRID_VERSION_PATCH > 0))
TEST_F(StandardJobExecutorSimulatedFailuresTest, RandomFailures) {
#else
    TEST_F(StandardJobExecutorSimulatedFailuresTest, DISABLED_RandomFailures) {
#endif
    DO_TEST_WITH_FORK(do_RandomFailures_test);
}

void StandardJobExecutorSimulatedFailuresTest::do_RandomFailures_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string stable_host = "StableHost";

    // Create a Compute Service that has access to two hosts
    compute_service = simulation->add(
            new wrench::StandardJobExecutor(stable_host,
                                                (std::map<std::string, std::tuple<unsigned long, double>>){
                                                        std::make_pair("FailedHost1", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
                                                        std::make_pair("FailedHost2", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)),
                                                },
                                                100.0,
                                                {}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(stable_host, 10000000000000.0));

    // Create a WMS
    wrench::WMS *wms = nullptr;
    wms = simulation->add(new RandomFailuresTestWMS(this, stable_host, compute_service, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(stable_host)));
    ASSERT_NO_THROW(simulation->stageFiles({{input_file->getID(), input_file}}, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}

#endif