/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/simulation/SimulationMessage.h>
#include <gtest/gtest.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include <wrench/job/PilotJob.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

#define EPSILON 0.05

WRENCH_LOG_CATEGORY(batch_service_fcfs_test, "Log category for BatchServiceFCFSTest");

class BatchServiceFCFSTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::BatchComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::Simulation> simulation;

    void do_SimpleFCFS_test();
    void do_SimpleFCFSQueueWaitTimePrediction_test();
    void do_BrokenQueueWaitTimePrediction_test();

    std::shared_ptr<wrench::Workflow> workflow;

protected:
    BatchServiceFCFSTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create a four-host 10-core platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\"/> "
                          "       <link id=\"1\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <link id=\"3\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <route src=\"Host3\" dst=\"Host1\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host4\" dst=\"Host1\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\""
                          "/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**  SIMPLE FCFS TEST                                                **/
/**********************************************************************/

class SimpleFCFSTestWMS : public wrench::ExecutionController {

public:
    SimpleFCFSTestWMS(BatchServiceFCFSTest *test,
                      std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceFCFSTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create 4 tasks and submit them as various shaped jobs

        std::shared_ptr<wrench::WorkflowTask> tasks[8];
        std::shared_ptr<wrench::StandardJob> jobs[8];
        for (int i = 0; i < 8; i++) {
            tasks[i] = this->test->workflow->addTask("task" + std::to_string(i), 60, 1, 1, 0);
            jobs[i] = job_manager->createStandardJob(tasks[i]);
        }

        std::map<std::string, std::string>
                two_hosts_ten_cores,
                two_hosts_five_cores,
                one_hosts_five_cores,
                four_hosts_five_cores;

        two_hosts_ten_cores["-N"] = "2";
        two_hosts_ten_cores["-t"] = "120";
        two_hosts_ten_cores["-c"] = "10";

        two_hosts_five_cores["-N"] = "2";
        two_hosts_five_cores["-t"] = "120";
        two_hosts_five_cores["-c"] = "5";

        one_hosts_five_cores["-N"] = "1";
        one_hosts_five_cores["-t"] = "120";
        one_hosts_five_cores["-c"] = "5";

        four_hosts_five_cores["-N"] = "4";
        four_hosts_five_cores["-t"] = "120";
        four_hosts_five_cores["-c"] = "5";

        std::map<std::string, std::string> job_args[8] = {
                two_hosts_ten_cores,
                four_hosts_five_cores,
                two_hosts_ten_cores,
                two_hosts_ten_cores,
                four_hosts_five_cores,
                two_hosts_five_cores,
                one_hosts_five_cores,
                four_hosts_five_cores};

        double expected_completion_times[8] = {
                60,
                120,
                180,
                180,
                240,
                240,
                240,
                300};

        // Submit jobs
        try {
            for (int i = 0; i < 8; i++) {
                job_manager->submitJob(jobs[i], this->test->compute_service, job_args[i]);
            }
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(
                    "Unexpected exception while submitting job");
        }

        double actual_completion_times[8];
        for (int i = 0; i < 8; i++) {
            // Wait for a workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                actual_completion_times[i] = wrench::Simulation::getCurrentSimulatedDate();
            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        // Check
        for (int i = 0; i < 8; i++) {
            double delta = std::abs(actual_completion_times[i] - expected_completion_times[i]);
            if (delta > EPSILON) {
                throw std::runtime_error("Unexpected job completion time for the job containing task " +
                                         tasks[i]->getID() +
                                         ": " +
                                         std::to_string(actual_completion_times[i]) +
                                         " (expected: " +
                                         std::to_string(expected_completion_times[i]) +
                                         ")");
            }
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceFCFSTest, DISABLED_SimpleFCFSTest)
#else
TEST_F(BatchServiceFCFSTest, SimpleFCFSTest)
#endif
{
    DO_TEST_WITH_FORK(do_SimpleFCFS_test);
}


void BatchServiceFCFSTest::do_SimpleFCFS_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service with a fcfs scheduling algorithm
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "fcfs"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new SimpleFCFSTestWMS(
                                    this, hostname)));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  SIMPLE FCFS TEST WITH QUEUE PREDICTION                          **/
/**********************************************************************/

class SimpleFCFSQueueWaitTimePredictionWMS : public wrench::ExecutionController {

public:
    SimpleFCFSQueueWaitTimePredictionWMS(BatchServiceFCFSTest *test,
                                         std::string hostname) : wrench::ExecutionController(hostname,
                                                                                             "test") {
        this->test = test;
    }

private:
    BatchServiceFCFSTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create 4 tasks and submit them as three various shaped jobs

        std::shared_ptr<wrench::WorkflowTask> tasks[9];
        std::shared_ptr<wrench::StandardJob> jobs[9];
        for (int i = 0; i < 9; i++) {
            tasks[i] = this->test->workflow->addTask("task" + std::to_string(i), 60, 1, 1, 0);
            jobs[i] = job_manager->createStandardJob(tasks[i]);
        }

        std::map<std::string, std::string>
                two_hosts_ten_cores,
                two_hosts_five_cores,
                one_hosts_five_cores,
                three_hosts_five_cores,
                one_host_four_cores_short;

        two_hosts_ten_cores["-N"] = "2";
        two_hosts_ten_cores["-t"] = "120";
        two_hosts_ten_cores["-c"] = "10";

        two_hosts_five_cores["-N"] = "2";
        two_hosts_five_cores["-t"] = "120";
        two_hosts_five_cores["-c"] = "5";

        one_hosts_five_cores["-N"] = "1";
        one_hosts_five_cores["-t"] = "120";
        one_hosts_five_cores["-c"] = "5";

        three_hosts_five_cores["-N"] = "3";
        three_hosts_five_cores["-t"] = "120";
        three_hosts_five_cores["-c"] = "5";

        one_host_four_cores_short["-N"] = "1";
        one_host_four_cores_short["-t"] = "60";
        one_host_four_cores_short["-c"] = "4";

        std::map<std::string, std::string> job_args[9] = {
                two_hosts_ten_cores,
                three_hosts_five_cores,
                two_hosts_ten_cores,
                two_hosts_ten_cores,
                three_hosts_five_cores,
                two_hosts_five_cores,
                one_hosts_five_cores,
                two_hosts_five_cores,
                one_host_four_cores_short};


        // Submit jobs
        try {
            for (int i = 0; i < 9; i++) {
                job_manager->submitJob(jobs[i], this->test->compute_service, job_args[i]);
            }
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(
                    "Unexpected exception while submitting job");
        }

        // Sleep for 10 seconds
        wrench::Simulation::sleep(10);

        // Get Predictions
        std::set<std::tuple<std::string, unsigned long, unsigned long, sg_size_t>> set_of_jobs = {
                (std::tuple<std::string, unsigned long, unsigned long, sg_size_t>){"job1", 1, 1, 400},
                (std::tuple<std::string, unsigned long, unsigned long, sg_size_t>){"job2", 5, 1, 400},
                (std::tuple<std::string, unsigned long, unsigned long, sg_size_t>){"job3", 4, 10, 400},
                (std::tuple<std::string, unsigned long, unsigned long, sg_size_t>){"job4", 1, 6, 400},
                (std::tuple<std::string, unsigned long, unsigned long, sg_size_t>){"job5", 2, 6, 400},
                (std::tuple<std::string, unsigned long, unsigned long, sg_size_t>){"job6", 2, 7, 400},
                (std::tuple<std::string, unsigned long, unsigned long, sg_size_t>){"job7", 3, 7, 400},
                (std::tuple<std::string, unsigned long, unsigned long, sg_size_t>){"job8", 4, 4, 400},
                (std::tuple<std::string, unsigned long, unsigned long, sg_size_t>){"job9", 1, 1, 400},
                (std::tuple<std::string, unsigned long, unsigned long, sg_size_t>){"job10", 1, 2, 400},
        };

        // Expectations
        std::map<std::string, double> expectations;
        expectations.insert(std::make_pair("job1", 480));
        expectations.insert(std::make_pair("job2", -1));
        expectations.insert(std::make_pair("job3", 600));
        expectations.insert(std::make_pair("job4", 480));
        expectations.insert(std::make_pair("job5", 480));
        expectations.insert(std::make_pair("job6", 480));
        expectations.insert(std::make_pair("job7", 600));
        expectations.insert(std::make_pair("job8", 540));
        expectations.insert(std::make_pair("job9", 480));
        expectations.insert(std::make_pair("job10", 480));

        std::map<std::string, double> jobs_estimated_start_times =
                this->test->compute_service->getStartTimeEstimates(set_of_jobs);

        for (auto job: set_of_jobs) {
            std::string id = std::get<0>(job);
            double estimated = jobs_estimated_start_times[id];
            double expected = expectations[id];
            if (std::abs(estimated - expected) > 1.0) {
                throw std::runtime_error("invalid prediction for job '" + id + "': got " +
                                         std::to_string(estimated) + " but expected is " + std::to_string(expected));
            }
        }

        wrench::Simulation::sleep(10);

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceFCFSTest, DISABLED_SimpleFCFSQueueWaitTimePrediction)
#else
TEST_F(BatchServiceFCFSTest, SimpleFCFSQueueWaitTimePrediction)
#endif
{
    DO_TEST_WITH_FORK(do_SimpleFCFSQueueWaitTimePrediction_test);
}


void BatchServiceFCFSTest::do_SimpleFCFSQueueWaitTimePrediction_test() {


    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service with a fcfs scheduling algorithm
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "fcfs"},
                                                             {wrench::BatchComputeServiceProperty::BATCH_RJMS_PADDING_DELAY, "0"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new SimpleFCFSQueueWaitTimePredictionWMS(this, hostname)));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  BROKEN TEST WITH QUEUE PREDICTION                               **/
/**********************************************************************/

class BrokenQueueWaitTimePredictionWMS : public wrench::ExecutionController {

public:
    BrokenQueueWaitTimePredictionWMS(BatchServiceFCFSTest *test,
                                     std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceFCFSTest *test;

    int main() override {

        // Sleep for 10 seconds
        wrench::Simulation::sleep(10);

        // Get Predictions
        std::set<std::tuple<std::string, unsigned long, unsigned long, sg_size_t>> set_of_jobs = {
                (std::tuple<std::string, unsigned long, unsigned long, sg_size_t>){"job1", 1, 1, 400}};


        try {
            std::map<std::string, double> jobs_estimated_start_times =
                    this->test->compute_service->getStartTimeEstimates(set_of_jobs);
            throw std::runtime_error("Should not have been able to get prediction for BESTFIT algorithm");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::FunctionalityNotAvailable>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got expected exception, but unexpected failure cause: " +
                                         e.getCause()->toString() + "(Expected: FunctionalityNotAvailable)");
            }
            e.getCause()->toString();// coverage
            if (cause->getService() != this->test->compute_service) {
                throw std::runtime_error("Got expected exception and cause type, but compute service is wrong");
            }
            if (cause->getFunctionalityName() != "start time estimates") {
                throw std::runtime_error("Got expected exception and cause type, but functionality name is wrong (" +
                                         cause->getFunctionalityName() + ")");
            }
            WRENCH_INFO("toString: %s", cause->toString().c_str());// for coverage
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceFCFSTest, DISABLED_BrokenQueueWaitTimePrediction)
#else
TEST_F(BatchServiceFCFSTest, BrokenQueueWaitTimePrediction)
#endif
{
    DO_TEST_WITH_FORK(do_BrokenQueueWaitTimePrediction_test);
}


void BatchServiceFCFSTest::do_BrokenQueueWaitTimePrediction_test() {


    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service with a fcfs scheduling algorithm
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "fcfs"},
                                                             {wrench::BatchComputeServiceProperty::HOST_SELECTION_ALGORITHM, "BESTFIT"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new BrokenQueueWaitTimePredictionWMS(
                                    this, hostname)));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
