
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"


WRENCH_LOG_CATEGORY(simulation_timestamp_file_read_test, "Log category for SimulationTimestampFileReadTest");


class SimulationTimestampFileReadTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;


    std::shared_ptr<wrench::DataFile> file_1;
    std::shared_ptr<wrench::DataFile> file_2;
    std::shared_ptr<wrench::DataFile> file_3;
    std::shared_ptr<wrench::DataFile> xl_file;

    std::shared_ptr<wrench::WorkflowTask> task1 = nullptr;

    void do_SimulationTimestampFileReadBasic_test();

protected:
    ~SimulationTimestampFileReadTest() {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    SimulationTimestampFileReadTest() {

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"1\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"1000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"other_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"1\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"other_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <link id=\"1\" bandwidth=\"1Gbps\" latency=\"10000us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

        workflow = wrench::Workflow::createWorkflow();

        file_1 = wrench::Simulation::addFile("file_1", 100ULL);
        file_2 = wrench::Simulation::addFile("file_2", 100ULL);
        file_3 = wrench::Simulation::addFile("file_3", 100ULL);

        xl_file = wrench::Simulation::addFile("xl_file", 1000000000ULL);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**            SimulationTimestampFileReadTestBasic                      **/
/**********************************************************************/

/*
 * Testing the basic functionality of the SimulationTimestampFileRead class.
 * This test ensures that SimulationTimestampFileReadStart, SimulationTimestampFileReadFailure,
 * and SimulationTimestampFileReadCompletion objects are added to their respective simulation
 * traces at the appropriate times.
 */
class SimulationTimestampFileReadBasicTestWMS : public wrench::ExecutionController {
public:
    SimulationTimestampFileReadBasicTestWMS(SimulationTimestampFileReadTest *test,
                                            std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampFileReadTest *test;

    int main() override {

        auto job_manager = this->createJobManager();

        this->test->task1 = this->test->workflow->addTask("task1", 10.0, 1, 1, 0);
        this->test->task1->addInputFile(this->test->file_1);
        this->test->task1->addInputFile(this->test->file_2);
        this->test->task1->addInputFile(this->test->file_3);
        this->test->task1->addInputFile(this->test->xl_file);
        auto job1 = job_manager->createStandardJob(this->test->task1,
                                                   {{this->test->file_1, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->file_1)},
                                                    {this->test->file_2, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->file_2)},
                                                    {this->test->file_3, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->file_3)},
                                                    {this->test->xl_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->xl_file)}});
        job_manager->submitJob(job1, this->test->compute_service);

        this->waitForAndProcessNextEvent();

        /*
         * expected outcome:
         * file_1 start
         * file_1 end
         * xl_file start
         * file_2 start
         * file_2 end
         * file_3 start
         * file_3 end
         * file_3 start
         * file_3_end
         * xl_file end
         */

        return 0;
    }
};

TEST_F(SimulationTimestampFileReadTest, SimulationTimestampFileReadBasicTest) {
    DO_TEST_WITH_FORK(do_SimulationTimestampFileReadBasic_test);
}

void SimulationTimestampFileReadTest::do_SimulationTimestampFileReadBasic_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string host1 = "Host1";

    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::BareMetalComputeService(host1,
                                                                                          {std::make_pair(
                                                                                                  host1,
                                                                                                  std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                                  wrench::ComputeService::ALL_RAM))},
                                                                                          {})));

    ASSERT_NO_THROW(storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(host1, {"/"},
                                                                                                               {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "infinity"}})));

    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService(host1)));

    std::shared_ptr<wrench::ExecutionController> wms;

    ASSERT_NO_THROW(wms = simulation->add(new SimulationTimestampFileReadBasicTestWMS(
                            this, host1)));


    //stage files
    std::set<std::shared_ptr<wrench::DataFile>> files_to_stage = {file_1, file_2, file_3, xl_file};

    for (auto const &f: files_to_stage) {
        ASSERT_NO_THROW(storage_service->createFile(f));
    }

    simulation->getOutput().enableFileReadWriteCopyTimestamps(true);

    ASSERT_NO_THROW(simulation->launch());

    int expected_start_timestamps = 4;
    int expected_failure_timestamps = 0;
    int expected_completion_timestamps = 4;

    auto start_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampFileReadStart>();
    auto failure_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampFileReadFailure>();
    auto completion_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampFileReadCompletion>();

    // check number of SimulationTimestampFileRead
    ASSERT_EQ(expected_start_timestamps, start_timestamps.size());
    ASSERT_EQ(expected_failure_timestamps, failure_timestamps.size());
    ASSERT_EQ(expected_completion_timestamps, completion_timestamps.size());

    wrench::SimulationTimestampFileRead *file_1_start = start_timestamps[0]->getContent();
    wrench::SimulationTimestampFileRead *file_1_end = completion_timestamps[0]->getContent();

    wrench::SimulationTimestampFileRead *file_2_start = start_timestamps[1]->getContent();
    wrench::SimulationTimestampFileRead *file_2_end = completion_timestamps[1]->getContent();

    wrench::SimulationTimestampFileRead *file_3_1_start = start_timestamps[2]->getContent();
    wrench::SimulationTimestampFileRead *file_3_1_end = completion_timestamps[2]->getContent();

    wrench::SimulationTimestampFileRead *xl_file_start = start_timestamps[3]->getContent();
    wrench::SimulationTimestampFileRead *xl_file_end = completion_timestamps[3]->getContent();

    // list of expected matching start and end timestamps
    std::vector<std::pair<wrench::SimulationTimestampFileRead *, wrench::SimulationTimestampFileRead *>> file_read_timestamps = {
            std::make_pair(file_1_start, file_1_end),
            std::make_pair(file_2_start, file_2_end),
            std::make_pair(file_3_1_start, file_3_1_end),
            std::make_pair(xl_file_start, xl_file_end),
    };

//    std::shared_ptr<wrench::StorageService> service = storage_service;

    for (auto &fc: file_read_timestamps) {

        // endpoints should be set correctly
        ASSERT_EQ(fc.first->getEndpoint(), fc.second);
        ASSERT_EQ(fc.second->getEndpoint(), fc.first);

        // completion/failure timestamp times should be greater than start timestamp times
        ASSERT_GT(fc.second->getDate(), fc.first->getDate());

        // source should be set
        ASSERT_EQ(this->storage_service, fc.first->getSource()->getStorageService());
        ASSERT_EQ("/", fc.first->getSource()->getDirectoryPath());

        ASSERT_EQ(this->storage_service, fc.second->getSource()->getStorageService());
        ASSERT_EQ("/", fc.second->getSource()->getDirectoryPath());

        //service should be set
        ASSERT_EQ(fc.first->getService(), fc.second->getService());

        // file should be set
        ASSERT_EQ(fc.first->getFile(), fc.second->getFile());

        //task1 should be set
        ASSERT_EQ(fc.first->getTask(), fc.second->getTask());
    }


    // test constructors for invalid arguments
#ifdef WRENCH_INTERNAL_EXCEPTIONS
    ASSERT_THROW(simulation->getOutput().addTimestampFileReadStart(0.0,
                                                                   nullptr,
                                                                   wrench::FileLocation::LOCATION(this->storage_service, file_1),
                                                                   service,
                                                                   task1),
                 std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileReadStart(0.0,
                                                                   file_1,
                                                                   nullptr,
                                                                   service,
                                                                   task1),
                 std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileReadStart(0.0,
                                                                   file_1,
                                                                   wrench::FileLocation::LOCATION(this->storage_service, file_1),
                                                                   nullptr,
                                                                   task1),
                 std::invalid_argument);


    ASSERT_THROW(simulation->getOutput().addTimestampFileReadFailure(0.0,
                                                                     nullptr,
                                                                     wrench::FileLocation::LOCATION(this->storage_service, file_1),
                                                                     service,
                                                                     task1),
                 std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileReadFailure(0.0,
                                                                     file_1,
                                                                     wrench::FileLocation::LOCATION(this->storage_service, file_1),
                                                                     nullptr,
                                                                     task1),
                 std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileReadCompletion(0.0,
                                                                        nullptr,
                                                                        nullptr,
                                                                        service,
                                                                        task1),
                 std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileReadCompletion(0.0,
                                                                        file_1,
                                                                        nullptr,
                                                                        service,
                                                                        task1),
                 std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileReadCompletion(0.0,
                                                                        file_1,
                                                                        wrench::FileLocation::LOCATION(this->storage_service, file_1),
                                                                        nullptr,
                                                                        task1),
                 std::invalid_argument);
#endif

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
