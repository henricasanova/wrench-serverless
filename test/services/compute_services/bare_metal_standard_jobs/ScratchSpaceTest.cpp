/**
 * Copyright (c) 2017-2021. The WRENCH Team.
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

WRENCH_LOG_CATEGORY(scratch_service_test, "Log category for ScratchServiceTest");


class ScratchSpaceTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::SimpleStorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service1 = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service2 = nullptr;
    std::shared_ptr<wrench::Simulation> simulation;

    void do_BogusScratchSpace_test();

    void do_SimpleScratchSpace_test();

    void do_ScratchSpaceFailure_test();

    void do_PilotJobScratchSpace_test();

    void do_RaceConditionTest_test();

    void do_PartitionsTest_test();

protected:
    ~ScratchSpaceTest() {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    ScratchSpaceTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create a four-host 10-core platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> ";
        for (int i = 1; i <= 4; i++) {
            xml += "       <host id=\"Host" + std::to_string(i) + "\" speed=\"1f\" core=\"10\" > "
                                                                  "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                                                  "             <prop id=\"size\" value=\"10000000000000B\"/>"
                                                                  "             <prop id=\"mount\" value=\"/disk1\"/>"
                                                                  "          </disk>"
                                                                  "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                                                  "             <prop id=\"size\" value=\"1000000000000B\"/>"
                                                                  "             <prop id=\"mount\" value=\"/disk2\"/>"
                                                                  "          </disk>"
                                                                  "          <disk id=\"disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                                                  "             <prop id=\"size\" value=\"3000B\"/>"
                                                                  "             <prop id=\"mount\" value=\"/scratch3000\"/>"
                                                                  "          </disk>"
                                                                  "          <disk id=\"disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                                                  "             <prop id=\"size\" value=\"10000B\"/>"
                                                                  "             <prop id=\"mount\" value=\"/scratch10000\"/>"
                                                                  "          </disk>"
                                                                  "          <disk id=\"disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                                                  "             <prop id=\"size\" value=\"100B\"/>"
                                                                  "             <prop id=\"mount\" value=\"/scratch100\"/>"
                                                                  "          </disk>"
                                                                  "       </host>";
        }
        xml += "       <link id=\"1\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
               "       <link id=\"2\" bandwidth=\"0.0001MBps\" latency=\"1000000us\"/>"
               "       <link id=\"3\" bandwidth=\"0.0001MBps\" latency=\"1000000us\"/>"
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
/**                     BOGUS  SCRATCH SPACE TEST                    **/
/**********************************************************************/

TEST_F(ScratchSpaceTest, BogusScratchSpaceTest) {
    DO_TEST_WITH_FORK(do_BogusScratchSpace_test);
}


void ScratchSpaceTest::do_BogusScratchSpace_test() {


    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                "/scratch_bogus", {})),
                 std::invalid_argument);


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**                     SIMPLE SCRATCH SPACE TEST                    **/
/**********************************************************************/

class SimpleScratchSpaceTestWMS : public wrench::ExecutionController {

public:
    SimpleScratchSpaceTestWMS(ScratchSpaceTest *test,
                              std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    ScratchSpaceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a sequential task1 that lasts one min and requires 1 cores
        std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task1", 60, 1, 1, 0);
        task->addInputFile(wrench::Simulation::getFileByID("input_file"));
        task->addOutputFile(wrench::Simulation::getFileByID("output_file"));

        // Create a StandardJob with some pre-copies
        auto job = job_manager->createStandardJob(
                {task},
                (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                {std::make_tuple(
                        wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")),
                        wrench::FileLocation::SCRATCH(wrench::Simulation::getFileByID("input_file")))},
                {},
                {});

        // Submit the job for execution
        job_manager->submitJob(job, this->test->compute_service);

        // Wait for a workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        if (std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            //sleep to make sure that the files are deleted
            wrench::S4U_Simulation::sleep(100);
            double free_space_size = this->test->compute_service->getFreeScratchSpaceSize();
            if (free_space_size < this->test->compute_service->getTotalScratchSpaceSize()) {
                throw std::runtime_error(
                        "File was not deleted from scratch");
            }
        } else {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        return 0;
    }
};

TEST_F(ScratchSpaceTest, SimpleScratchSpaceTest) {
    DO_TEST_WITH_FORK(do_SimpleScratchSpace_test);
}


void ScratchSpaceTest::do_SimpleScratchSpace_test() {


    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));


    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                          wrench::ComputeService::ALL_RAM))},
                                                "/scratch3000",
                                                {})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
            new SimpleScratchSpaceTestWMS(
                    this, hostname)));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 1000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 2000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**                  SIMPLE SCRATCH SPACE FAILURE TEST               **/
/**********************************************************************/

class SimpleScratchSpaceFailureTestWMS : public wrench::ExecutionController {

public:
    SimpleScratchSpaceFailureTestWMS(ScratchSpaceTest *test,
                                     std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    ScratchSpaceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a sequential task that lasts one min and requires 1 cores
            std::shared_ptr<wrench::WorkflowTask> task1 = this->test->workflow->addTask("task1", 60, 1, 1, 0);
            task1->addInputFile(wrench::Simulation::getFileByID("input_file1"));

            // Create a sequential task that lasts one min and requires 1 cores
            std::shared_ptr<wrench::WorkflowTask> task2 = this->test->workflow->addTask("task2", 60, 1, 1, 0);
            task2->addInputFile(wrench::Simulation::getFileByID("input_file2"));

            // Create a StandardJob with SOME pre-copies from public storage to scratch
            auto job1 = job_manager->createStandardJob(
                    {task1},
                    (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                    {std::make_tuple(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file1")),
                            wrench::FileLocation::SCRATCH(wrench::Simulation::getFileByID("input_file1")))},
                    {},
                    {});


            // Create a StandardJob with NO pre-copies from public storage to scratch
            auto job2 = job_manager->createStandardJob(
                    {task2},
                    (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                    {std::make_tuple(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file2")),
                            wrench::FileLocation::SCRATCH(wrench::Simulation::getFileByID("input_file2")))},
                    {},
                    {});

            // Submit the job for execution to the compute service with NO scratch, thus expecting an error
            try {
                job_manager->submitJob(job1, this->test->compute_service);
                throw std::runtime_error("Should not have been able to submit job 1 (first attempt)");
            } catch (std::invalid_argument &ignore) {
            }

            // Submit the job for execution to the compute service which has some scratch, but not enough space, thus expecting an exception
            job1 = job_manager->createStandardJob(
                    {task1},
                    (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                    {std::make_tuple(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file1")),
                            wrench::FileLocation::SCRATCH(wrench::Simulation::getFileByID("input_file1")))},
                    {},
                    {});
            job_manager->submitJob(job1, this->test->compute_service1);
            // Wait for a workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }

            auto real_event2 = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);
            if (real_event2) {
                auto cause = std::dynamic_pointer_cast<wrench::StorageServiceNotEnoughSpace>(real_event2->failure_cause);
                if (not cause) {
                    throw std::runtime_error("Got a job failure event, but unexpected failure cause: " +
                                             real_event2->failure_cause->toString() + " (expected: StorageServiceNotEnoughSpace)");
                }
            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            // Submit two jobs for execution to the same compute service which has just enough scratch for only one job and so
            // one should succeed and the second one should fail
            job1 = job_manager->createStandardJob(
                    {task1},
                    (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                    {std::make_tuple(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file1")),
                            wrench::FileLocation::SCRATCH(wrench::Simulation::getFileByID("input_file1")))},
                    {},
                    {});
            job_manager->submitJob(job1, this->test->compute_service2);
            wrench::S4U_Simulation::sleep(1);

            job_manager->submitJob(job2, this->test->compute_service2);
            // Wait for a workflow execution event
            int num_events = 0;
            int prev_event = -1;
            int i = 0;
            while (i < 2) {
                try {
                    event = this->waitForNextEvent();
                } catch (wrench::ExecutionException &e) {
                    throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
                }

                if (std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                    if (prev_event == -1) {
                        prev_event = 0;
                        num_events++;
                    } else if (prev_event == 1) {
                        num_events++;
                    }
                } else if (std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
                    if (prev_event == -1) {
                        prev_event = 1;
                        num_events++;
                    } else if (prev_event == 0) {
                        num_events++;
                    }
                } else {
                    throw std::runtime_error("Unexpected workflow execution event or here: " + event->toString());
                }
                i++;
            }

            if (num_events != 2) {
                throw std::runtime_error("Did not get enough execution events");
            }
        }

        return 0;
    }
};

TEST_F(ScratchSpaceTest, SimpleScratchSpaceFailureTest) {
    DO_TEST_WITH_FORK(do_ScratchSpaceFailure_test);
}


void ScratchSpaceTest::do_ScratchSpaceFailure_test() {


    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service3
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"},
                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"},
                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}})));


    // Create a Compute Service that does not have scratch space
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                          wrench::ComputeService::ALL_RAM))},
                                                "")));

    // Create a Compute Service that has smaller scratch space than the files to be stored
    ASSERT_NO_THROW(compute_service1 = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                          wrench::ComputeService::ALL_RAM))},
                                                "/scratch100")));

    // Create a Compute Service that has enough scratch space to store the files
    ASSERT_NO_THROW(compute_service2 = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                          wrench::ComputeService::ALL_RAM))},
                                                "/scratch10000")));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
            new SimpleScratchSpaceFailureTestWMS(
                    this, hostname)));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file1 = wrench::Simulation::addFile("input_file1", 10000);
    std::shared_ptr<wrench::DataFile> input_file2 = wrench::Simulation::addFile("input_file2", 10000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file1));
    ASSERT_NO_THROW(storage_service1->createFile(input_file2));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**             SCRATCH SPACE FAILURE TEST FOR PILOT JOBS            **/
/**********************************************************************/

class PilotJobScratchSpaceTestWMS : public wrench::ExecutionController {

public:
    PilotJobScratchSpaceTestWMS(ScratchSpaceTest *test,
                                std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    ScratchSpaceTest *test;

    int main() override {

        // Create a job  manager
        auto job_manager = this->createJobManager();

        // Create a pilot job that requires 1 host, 1 core per host, 1 bytes of RAM per host, and 1 hour
        auto pilot_job = job_manager->createPilotJob();

        // Submit a pilot job
        try {
            job_manager->submitJob(pilot_job, this->test->compute_service, {{"-N", "1"}, {"-c", "1"}, {"-t", "3600"}});
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Unexpected exception: " + e.getCause()->toString());
        }

        // Wait for the pilot job start
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(
                    "Error while getting and execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::PilotJobStartedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Create a sequential task that lasts one min and requires 1 cores
        std::shared_ptr<wrench::WorkflowTask> task1 = this->test->workflow->addTask("task1", 60, 1, 1, 0);
        task1->addInputFile(wrench::Simulation::getFileByID("input_file1"));

        // Create a sequential task that lasts one min and requires 1 cores
        std::shared_ptr<wrench::WorkflowTask> task2 = this->test->workflow->addTask("task2", 360, 1, 1, 0);
        task2->addInputFile(wrench::Simulation::getFileByID("input_file2"));

        // Create a sequential task that lasts one min and requires 1 cores
        std::shared_ptr<wrench::WorkflowTask> task3 = this->test->workflow->addTask("task3", 600, 1, 1, 0);
        task3->addInputFile(wrench::Simulation::getFileByID("input_file3"));

        // Create a StandardJob with SOME pre-copies from public storage to scratch
        auto job1 = job_manager->createStandardJob(
                {task1},
                (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                {std::make_tuple(
                        wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file1")),
                        wrench::FileLocation::SCRATCH(wrench::Simulation::getFileByID("input_file1")))},
                {},
                {});

        // Create a StandardJob with SOME pre-copies from public storage to scratch
        auto job2 = job_manager->createStandardJob(
                {task2},
                (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                {std::make_tuple(
                        wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file2")),
                        wrench::FileLocation::SCRATCH(wrench::Simulation::getFileByID("input_file2")))},
                {},
                {});

        // Create a StandardJob with SOME pre-copies from public storage to scratch
        auto job3 = job_manager->createStandardJob(
                {task3},
                (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                {std::make_tuple(
                        wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file3")),
                        wrench::FileLocation::SCRATCH(wrench::Simulation::getFileByID("input_file3")))},
                {},
                {});


        // Submit the standard job for execution
        try {
            job_manager->submitJob(job1, pilot_job->getComputeService());
            job_manager->submitJob(job2, pilot_job->getComputeService());
            job_manager->submitJob(job3, pilot_job->getComputeService());
        } catch (std::exception &e) {
            throw std::runtime_error(
                    "Unexpected exception while submitting standard job to pilot job: " + std::string(e.what()));
        }

        int i = 0;
        while (i < 3) {
            // Wait for the standard job completion
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Error while getting and execution event: " + e.getCause()->toString());
            }
            if (std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {

            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
            i++;
        }

        // Wait for the pilot job expiration
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(
                    "Error while getting and execution event: " + e.getCause()->toString());
        }

        if (std::dynamic_pointer_cast<wrench::PilotJobExpiredEvent>(event)) {
            // success, check if the scratch space size is full again or not, it should be full
            wrench::S4U_Simulation::sleep(10);//sleep for some time to ensure everything is deleted
            double free_space_size = pilot_job->getComputeService()->getFreeScratchSpaceSize();
            if (free_space_size != 3000.0) {
                throw std::runtime_error(
                        "Scratch space should be empty after this pilot job expires (expected: 3000.00  vs. actual: " +
                        std::to_string(free_space_size));
            }
        } else {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(ScratchSpaceTest, PilotJobScratchSpaceTest) {
#else
TEST_F(ScratchSpaceTest, PilotJobScratchSpaceTest) {
#endif
    DO_TEST_WITH_FORK(do_PilotJobScratchSpace_test);
}


void ScratchSpaceTest::do_PilotJobScratchSpace_test() {


    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //      argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));


    // Create a Compute Service that does not have scratch space
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {hostname}, "/scratch3000", {})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
            new PilotJobScratchSpaceTestWMS(
                    this, hostname)));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file1 = wrench::Simulation::addFile("input_file1", 1000);
    std::shared_ptr<wrench::DataFile> input_file2 = wrench::Simulation::addFile("input_file2", 1000);
    std::shared_ptr<wrench::DataFile> input_file3 = wrench::Simulation::addFile("input_file3", 1000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file1));
    ASSERT_NO_THROW(storage_service1->createFile(input_file2));
    ASSERT_NO_THROW(storage_service1->createFile(input_file3));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**           RACE CONDITION TEST                                    **/
/**********************************************************************/

class ScratchSpaceRaceConditionTestWMS : public wrench::ExecutionController {

public:
    ScratchSpaceRaceConditionTestWMS(ScratchSpaceTest *test,
                                     std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    ScratchSpaceTest *test;

    int main() override {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Get a reference to the file
        std::shared_ptr<wrench::DataFile> file = wrench::Simulation::getFileByID("input");

        // Create three tasks
        std::shared_ptr<wrench::WorkflowTask> task1 = this->test->workflow->addTask("task1", 10, 1, 1, 0);// 10 seconds
        std::shared_ptr<wrench::WorkflowTask> task2 = this->test->workflow->addTask("task2", 10, 1, 1, 0);// 10 seconds
        this->test->workflow->addControlDependency(task1, task2);                                         // task1 1 depends on task2
        task2->addInputFile(file);

        std::shared_ptr<wrench::WorkflowTask> task3 = this->test->workflow->addTask("task3", 1, 1, 1, 0);// 1 second


        // Create a first job that:
        //   - copies file "input" to the scratch space
        //   - runs task1 and then task2 (10 second each)
        //   - (task 2 needs "input")
        auto job1 = job_manager->createStandardJob(
                {task1, task2}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                {std::make_tuple(
                        wrench::FileLocation::LOCATION(this->test->storage_service1, file),
                        wrench::FileLocation::SCRATCH(file))},
                {}, {});

        // Create a second job that:
        //    - copies file "input" to the scratch space
        //    - runs task3 (1 second)
        auto job2 = job_manager->createStandardJob(
                {task3}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                {std::make_tuple(
                        wrench::FileLocation::LOCATION(this->test->storage_service1, file),
                        wrench::FileLocation::SCRATCH(file))},
                {}, {});

        // Submit both jobs
        job_manager->submitJob(job1, this->test->compute_service);
        job_manager->submitJob(job2, this->test->compute_service);


        // Wait for workflow execution events
        for (auto job: {job1, job2}) {
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        return 0;
    }
};

TEST_F(ScratchSpaceTest, RaceConditionTest) {
    DO_TEST_WITH_FORK(do_RaceConditionTest_test);
}

void ScratchSpaceTest::do_RaceConditionTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"},
                                                                     {},
                                                                     {{wrench::SimpleStorageServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 3.0}})));

    // Create a Cloud Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname, {"Host1"}, "/scratch3000", {}, {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
            new ScratchSpaceRaceConditionTestWMS(this, hostname)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Create a file
    std::shared_ptr<wrench::DataFile> file = nullptr;
    ASSERT_NO_THROW(file = wrench::Simulation::addFile("input", 1));
    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(file));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**     Partitions Test (For both Sratch and Non-Scratch)           **/
/**********************************************************************/

class ScratchNonScratchPartitionsTestWMS : public wrench::ExecutionController {

public:
    ScratchNonScratchPartitionsTestWMS(ScratchSpaceTest *test,
                                       std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    ScratchSpaceTest *test;

    int main() override {

        //NonScratch have only / partition but other partitions can be created
        //Scratch have /, /<job's_name> partitions

        // Create a data movement manager and this should only copy from / to / of two non scratch space
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Get a reference to the file
        std::shared_ptr<wrench::DataFile> file1 = wrench::Simulation::getFileByID("input1");
        // Get a reference to the file
        std::shared_ptr<wrench::DataFile> file2 = wrench::Simulation::getFileByID("input2");

        //check if this file is staged at mount point of non-scratch
        if (not wrench::StorageService::lookupFileAtLocation(
                wrench::FileLocation::LOCATION(test->storage_service1, file1))) {
            throw std::runtime_error(
                    "The file1 was supposed to be staged at the mount point but is not");
        }
        //check if this file is staged at mount point of non-scratch
        if (not wrench::StorageService::lookupFileAtLocation(
                wrench::FileLocation::LOCATION(test->storage_service2, file2))) {
            throw std::runtime_error(
                    "The file2 was supposed to be staged in / partition but is not");
        }

        // Create a task
        std::shared_ptr<wrench::WorkflowTask> task1 = this->test->workflow->addTask("task1", 10, 1, 1, 0);// 10 seconds
        task1->addInputFile(file1);

        // Create a first job that:
        //   - copies file "input" to the scratch space
        //   - runs task1
        auto job1 = job_manager->createStandardJob(
                {task1}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                {std::make_tuple(
                        wrench::FileLocation::LOCATION(this->test->storage_service1, file1),
                        wrench::FileLocation::SCRATCH(file1))},
                {}, {});

        // Submit job1
        job_manager->submitJob(job1, this->test->compute_service);

        // Wait for workflow execution events
        for (auto job: {job1}) {
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }


        //the file1 should still be non-scratch space, the job should only delete file from it's scratch job's partition
        //check if this file is staged in mount point of non-scratch
        if (not wrench::StorageService::lookupFileAtLocation(
                wrench::FileLocation::LOCATION(test->storage_service1, file1))) {
            throw std::runtime_error(
                    "The file1 again was supposed to be staged in / partition but is not");
        }

        //try to copy file1 from job1's partition of storage service1 into storage service2 in / partition, this should fail
        try {
            wrench::StorageService::copyFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service1,
                                                   this->test->storage_service1->getBaseRootPath() + job1->getName(), file1),
                    wrench::FileLocation::LOCATION(this->test->storage_service2, file1));
            throw std::runtime_error(
                    "Non-scratch space have / partition unless created by copying something into a new partition name");
        } catch (wrench::ExecutionException &e) {
        }

        //try to copy file1 from / partition of storage service1 into storage service2 in job1's partition, this should succeed
        try {
            wrench::StorageService::copyFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service1, file1),
                    wrench::FileLocation::LOCATION(this->test->storage_service2,
                                                   this->test->storage_service2->getBaseRootPath() + job1->getName(), file1));

        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(
                    "We should have been able to copy from / partition of non-scratch to a new partition into another non-scratch space");
        }

        //try to copy file2 from / partition of storage service2 into storage service1 at mount point, it should succeed
        try {
            wrench::StorageService::copyFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service2, file2),
                    wrench::FileLocation::LOCATION(this->test->storage_service1, file2));

        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(
                    "We should have been able to copy from / of one non-scratch space to / of another non-scratch space");
        }

        //try to copy file2 from / partition of storage service2 into storage service2 in /test directory, it should succeed
        try {
            wrench::StorageService::copyFile(
                    wrench::FileLocation::LOCATION(this->test->storage_service2, file2),
                    wrench::FileLocation::LOCATION(this->test->storage_service2,
                                                   this->test->storage_service2->getBaseRootPath() + "/test", file2));

        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(
                    "We should have been able to copy from one partition to another partition of the same storage service");
        }

        //we just copied file to /test partition of storage service2, so it must be there
        if (not wrench::StorageService::lookupFileAtLocation(
                wrench::FileLocation::LOCATION(this->test->storage_service2, this->test->storage_service2->getBaseRootPath() + "/test", file2))) {

            throw std::runtime_error(
                    "The file2 was supposed to be stored in /test partition but is not");
        }


        return 0;
    }
};

TEST_F(ScratchSpaceTest, ScratchNonScratchPartitionsTest) {
    DO_TEST_WITH_FORK(do_PartitionsTest_test);
}

void ScratchSpaceTest::do_PartitionsTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //        argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"},
                                                                     {},
                                                                     {{wrench::SimpleStorageServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1.0}})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"},
                                                                     {},
                                                                     {{wrench::SimpleStorageServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 3.0}})));

    // Create a Cloud Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname, {"Host1"}, "/scratch3000", {}, {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
            new ScratchNonScratchPartitionsTestWMS(this, hostname)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Create a file
    std::shared_ptr<wrench::DataFile> file1 = nullptr;
    ASSERT_NO_THROW(file1 = wrench::Simulation::addFile("input1", 1));
    // Create a file
    std::shared_ptr<wrench::DataFile> file2 = nullptr;
    ASSERT_NO_THROW(file2 = wrench::Simulation::addFile("input2", 1));
    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(file1));
    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service2->createFile(file2));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
