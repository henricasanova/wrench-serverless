/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include <wrench/action/FileReadAction.h>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/failure_causes/HostError.h>

#include <memory>

#include "../../../include/TestWithFork.h"
#include "../../../include/RuntimeAssert.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(file_read_action_executor_test, "Log category for FileReadActionExecutorTest");

//#define EPSILON (std::numeric_limits<double>::epsilon())
#define EPSILON (0.0001)

class FileReadActionExecutorTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Simulation> simulation;

    void do_FileReadActionExecutorSuccessTest_test();
    void do_FileReadActionExecutorMultipleAttemptsSuccessTest_test();
    void do_FileReadActionExecutorMissingFileTest_test();
    void do_FileReadActionExecutorKillingStorageServiceTest_test(double buffer_size);


protected:
    FileReadActionExecutorTest() {

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk1/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk2/\"/>"
                          "          </disk>"

                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\">  "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "         <prop id=\"ram\" value=\"1024B\"/> "
                          "       </host>  "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"0.1MBps\" latency=\"10us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "   </zone> "
                          "</platform>";
        // Create a four-host 10-core platform file
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::shared_ptr<wrench::Workflow> workflow;

public:
    std::shared_ptr<wrench::DataFile> file;
    std::shared_ptr<wrench::StorageService> ss;
};


/**********************************************************************/
/**  DO FILE_READ ACTION EXECUTOR SUCCESS TEST                       **/
/**********************************************************************/

class FileReadActionExecutorSuccessTestWMS : public wrench::ExecutionController {

public:
    FileReadActionExecutorSuccessTestWMS(FileReadActionExecutorTest *test,
                                         std::shared_ptr<wrench::Workflow> workflow,
                                         std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test), workflow(workflow) {
    }

private:
    FileReadActionExecutorTest *test;
    std::shared_ptr<wrench::Workflow> workflow;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");

        try {
            job->addFileReadAction("",
                                   wrench::FileLocation::LOCATION(this->test->ss, this->test->file),
                                   this->test->file->getSize() + 10);
            throw std::runtime_error("Shouldn't be able to read more bytes than the file contains");
        } catch (std::invalid_argument &ignore) {}

        // Add a file_read_action
        auto file_read_action = job->addFileReadAction("", wrench::FileLocation::LOCATION(this->test->ss, this->test->file));

        // coverage
        wrench::Action::getActionTypeAsString(file_read_action);
        file_read_action->getNumBytesToRead();
        file_read_action->getFile();
        file_read_action->getFileLocations();

        // Create a file read action executor
        auto file_read_action_executor = std::shared_ptr<wrench::ActionExecutor>(
                new wrench::ActionExecutor("Host2", 0, 0.0, 0, false, this->commport, nullptr, file_read_action, nullptr));

        // Start it
        file_read_action_executor->setSimulation(this->getSimulation());
        file_read_action_executor->start(file_read_action_executor, true, false);

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause());
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        // Is the start-date sensible?
        RUNTIME_DBL_EQ(file_read_action->getStartDate(), 0.0, "action start date", EPSILON);

        // Is the end-date sensible?
        RUNTIME_DBL_EQ(file_read_action->getEndDate(), 10.8349, "action end date", EPSILON);

        // Is the state sensible?
        RUNTIME_EQ(file_read_action->getState(), wrench::Action::State::COMPLETED, "action state");

        return 0;
    }
};

TEST_F(FileReadActionExecutorTest, SuccessTest) {
    DO_TEST_WITH_FORK(do_FileReadActionExecutorSuccessTest_test);
}


void FileReadActionExecutorTest::do_FileReadActionExecutorSuccessTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Storage Service
    this->ss = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(
            "Host3", {"/"},
            {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}}));

    // Create a workflow
    workflow = wrench::Workflow::createWorkflow();

    // Create a file
    this->file = wrench::Simulation::addFile("some_file", 1000000);

    ss->createFile(wrench::FileLocation::LOCATION(ss, file));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    wms = simulation->add(new FileReadActionExecutorSuccessTestWMS(this, workflow, "Host1"));

    ASSERT_NO_THROW(simulation->launch());

    workflow->clear();
    wrench::Simulation::removeAllFiles();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DO FILE_READ ACTION EXECUTOR MULTIPLE ATTEMPT SUCCESS TEST      **/
/**********************************************************************/

class FileReadActionExecutorMultipleAttemptsSuccessTestWMS : public wrench::ExecutionController {

public:
    FileReadActionExecutorMultipleAttemptsSuccessTestWMS(FileReadActionExecutorTest *test,
                                                         std::shared_ptr<wrench::Workflow> workflow,
                                                         std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test), workflow(workflow) {
    }

private:
    FileReadActionExecutorTest *test;
    std::shared_ptr<wrench::Workflow> workflow;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");

        // Add a file read action with bogus locations
        try {
            auto other_file = this->getSimulation()->addFile("other_file", 1);
            job->addFileReadAction("",
                                   {wrench::FileLocation::LOCATION(this->test->ss, "/bogus/", this->test->file),
                                    wrench::FileLocation::LOCATION(this->test->ss, other_file)});
            throw std::runtime_error("Shouldn't be able to add a file-read action with a vector of locations that are not for the same file");
        } catch (std::invalid_argument &ignore) {}

        // Add a file read action
        auto file_read_action = job->addFileReadAction("",
                                                       {wrench::FileLocation::LOCATION(this->test->ss, "/bogus/", this->test->file),
                                                        wrench::FileLocation::LOCATION(this->test->ss, this->test->file)});
        // Create a file read action executor
        auto file_read_action_executor = std::shared_ptr<wrench::ActionExecutor>(
                new wrench::ActionExecutor("Host2", 0, 0.0, 0, false, this->commport, nullptr, file_read_action, nullptr));
        // Start it
        file_read_action_executor->setSimulation(this->getSimulation());
        file_read_action_executor->start(file_read_action_executor, true, false);

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause());
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        // Is the state sensible?
        RUNTIME_EQ(file_read_action->getState(), wrench::Action::State::COMPLETED, "action state");
        //        if (file_read_action->getState() != wrench::Action::State::COMPLETED) {
        //            throw std::runtime_error("Unexpected action state: " + file_read_action->getStateAsString());
        //        }

        return 0;
    }
};

TEST_F(FileReadActionExecutorTest, MultipleAttemptsSuccessTest) {
    DO_TEST_WITH_FORK(do_FileReadActionExecutorMultipleAttemptsSuccessTest_test);
}


void FileReadActionExecutorTest::do_FileReadActionExecutorMultipleAttemptsSuccessTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Storage Service
    this->ss = simulation->add(wrench::SimpleStorageService::createSimpleStorageService("Host3", {"/"}));

    // Create a workflow
    workflow = wrench::Workflow::createWorkflow();

    // Create a file
    this->file = wrench::Simulation::addFile("some_file", 1000000);

    ss->createFile(wrench::FileLocation::LOCATION(ss, file));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    wms = simulation->add(new FileReadActionExecutorMultipleAttemptsSuccessTestWMS(this, workflow, "Host1"));

    ASSERT_NO_THROW(simulation->launch());

    workflow->clear();
    wrench::Simulation::removeAllFiles();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DO FILE_READ ACTION MISSING FILE TEST                       **/
/**********************************************************************/


class FileReadActionExecutorMissingFileTestWMS : public wrench::ExecutionController {

public:
    FileReadActionExecutorMissingFileTestWMS(FileReadActionExecutorTest *test,
                                             std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }


private:
    FileReadActionExecutorTest *test;
    double sleep_before_fail;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a file_read_action
        auto file_read_action = job->addFileReadAction("",
                                                       wrench::FileLocation::LOCATION(this->test->ss, this->test->file));
        // Create a file read action executor
        auto file_read_action_executor = std::shared_ptr<wrench::ActionExecutor>(
                new wrench::ActionExecutor("Host2", 0, 0.0, 0, false, this->commport, nullptr, file_read_action, nullptr));
        // Start it
        file_read_action_executor->setSimulation(this->getSimulation());
        file_read_action_executor->start(file_read_action_executor, true, false);

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause());
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        // Do we have the expected action state
        if (file_read_action->getState() != wrench::Action::State::FAILED) {
            throw std::runtime_error("Unexpected state: " + file_read_action->getStateAsString());
        }

        // Do we have a failure cause
        if (file_read_action->getFailureCause() == nullptr) {
            throw std::runtime_error("Failure cause should not be null");
        }

        // Do we have the expected failure cause
        if (not std::dynamic_pointer_cast<wrench::FileNotFound>(file_read_action->getFailureCause())) {
            throw std::runtime_error("Unexpected failure cause: " + file_read_action->getFailureCause()->toString());
        }


        return 0;
    }
};

TEST_F(FileReadActionExecutorTest, MissingFileTest) {
    DO_TEST_WITH_FORK(do_FileReadActionExecutorMissingFileTest_test);
}

void FileReadActionExecutorTest::do_FileReadActionExecutorMissingFileTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
    //    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Storage Service
    this->ss = simulation->add(wrench::SimpleStorageService::createSimpleStorageService("Host3", {"/"}, {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}}));

    // Create a workflow
    workflow = wrench::Workflow::createWorkflow();

    // Create a file
    this->file = wrench::Simulation::addFile("some_file", 1000000);

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new FileReadActionExecutorMissingFileTestWMS(this, "Host1")));

    ASSERT_NO_THROW(simulation->launch());

    workflow->clear();
    wrench::Simulation::removeAllFiles();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DO FILE_READ ACTION KILLING SS TEST                             **/
/**********************************************************************/


class FileReadActionExecutorKillingStorageServiceTestWMS : public wrench::ExecutionController {

public:
    FileReadActionExecutorKillingStorageServiceTestWMS(FileReadActionExecutorTest *test,
                                                       std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }


private:
    FileReadActionExecutorTest *test;
    double sleep_before_fail;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a file_read_action
        auto file_read_action = job->addFileReadAction("",
                                                       wrench::FileLocation::LOCATION(this->test->ss, this->test->file));
        // Create a file read action executor
        auto file_read_action_executor = std::shared_ptr<wrench::ActionExecutor>(
                new wrench::ActionExecutor("Host2", 0, 0.0, 0, false, this->commport, nullptr, file_read_action, nullptr));
        // Start it
        file_read_action_executor->setSimulation(this->getSimulation());
        file_read_action_executor->start(file_read_action_executor, true, false);

        // Sleep 1 sec
        wrench::Simulation::sleep(1.0);

        // Kill the SS
        simgrid::s4u::Host::by_name("Host3")->turn_off();

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause());
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }


        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        // Do we have the expected action state
        if (file_read_action->getState() != wrench::Action::State::FAILED) {
            throw std::runtime_error("Unexpected state: " + file_read_action->getStateAsString());
        }

        // Do we have a failure cause
        if (file_read_action->getFailureCause() == nullptr) {
            throw std::runtime_error("Failure cause should not be null");
        }

        // Do we have the expected failure cause
        if (not std::dynamic_pointer_cast<wrench::NetworkError>(file_read_action->getFailureCause())) {
            throw std::runtime_error("Unexpected failure cause: " + file_read_action->getFailureCause()->toString());
        }

        return 0;
    }
};

TEST_F(FileReadActionExecutorTest, KillingStorageServiceTest) {
    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillingStorageServiceTest_test, 1000000);
    //    DO_TEST_WITH_FORK_ONE_ARG(do_FileReadActionExecutorKillingStorageServiceTest_test, 0);
}

void FileReadActionExecutorTest::do_FileReadActionExecutorKillingStorageServiceTest_test(double buffer_size) {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();

    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
    //    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Storage Service
    this->ss = simulation->add(wrench::SimpleStorageService::createSimpleStorageService("Host3", {"/"},
                                                                                        {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {}));

    // Create a workflow
    workflow = wrench::Workflow::createWorkflow();

    // Create a file
    this->file = wrench::Simulation::addFile("some_file", 1000000);

    ss->createFile(wrench::FileLocation::LOCATION(ss, file));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new FileReadActionExecutorKillingStorageServiceTestWMS(this, "Host1")));

    ASSERT_NO_THROW(simulation->launch());

    workflow->clear();
    wrench::Simulation::removeAllFiles();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
