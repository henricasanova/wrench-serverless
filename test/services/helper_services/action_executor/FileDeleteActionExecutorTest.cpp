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

#include <wrench/action/FileDeleteAction.h>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/job/CompoundJob.h>

#include <memory>

#include "../../../include/RuntimeAssert.h"
#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(file_delete_action_executor_test, "Log category for FileDeleteActionExecutorTest");

//#define EPSILON (std::numeric_limits<double>::epsilon())
#define EPSILON (0.000001)


class FileDeleteActionExecutorTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Simulation> simulation;

    void do_FileDeleteActionExecutorSuccessTest_test();


protected:
    FileDeleteActionExecutorTest() {

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

public:
    std::shared_ptr<wrench::DataFile> file;
    std::shared_ptr<wrench::StorageService> ss;
};


/**********************************************************************/
/**  DO FILE_READ ACTION EXECUTOR SUCCESS TEST                       **/
/**********************************************************************/

class FileDeleteActionExecutorSuccessTestWMS : public wrench::ExecutionController {

public:
    FileDeleteActionExecutorSuccessTestWMS(FileDeleteActionExecutorTest *test,
                                           const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    FileDeleteActionExecutorTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");
        // Add a file_delete_action
        auto file_delete_action = job->addFileDeleteAction("", this->test->file,
                                                           this->test->ss);
        // coverage
        wrench::Action::getActionTypeAsString(file_delete_action);
        file_delete_action->getFile();
        file_delete_action->getFileLocation();
        file_delete_action->usesScratch();

        // Create a file read action executor
        auto file_delete_action_executor = std::make_shared<wrench::ActionExecutor>(
                "Host2", 0, 0.0, 0, false, this->commport, nullptr, file_delete_action, nullptr);
        // Start it
        file_delete_action_executor->setSimulation(this->getSimulation());
        file_delete_action_executor->start(file_delete_action_executor, true, false);

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

        // Is the start/end-date sensible?
        RUNTIME_DBL_EQ(file_delete_action->getStartDate(), 0.0, "action start date", EPSILON);
        RUNTIME_DBL_EQ(file_delete_action->getEndDate(), 0.216755, "action end date", EPSILON);

        // Is the state sensible
        RUNTIME_EQ(file_delete_action->getState(), wrench::Action::State::COMPLETED, "action state");

        return 0;
    }
};

TEST_F(FileDeleteActionExecutorTest, SuccessTest) {
    DO_TEST_WITH_FORK(do_FileDeleteActionExecutorSuccessTest_test);
}


void FileDeleteActionExecutorTest::do_FileDeleteActionExecutorSuccessTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-default-control-message-size=10000");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Storage Service
    this->ss = simulation->add(wrench::SimpleStorageService::createSimpleStorageService("Host3", {"/"}));

    // Create a file
    this->file = wrench::Simulation::addFile("some_file", 1000000);

    ss->createFile(wrench::FileLocation::LOCATION(ss, file));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    simulation->add(new FileDeleteActionExecutorSuccessTestWMS(this, "Host1"));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
