/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <cmath>

#include <gtest/gtest.h>

#include <wrench-dev.h>

#include "NoopScheduler.h"
#include "TestWithFork.h"


#define FILE_SIZE 10000000000.00
#define STORAGE_SIZE (100.0 * FILE_SIZE)

class SimpleStorageServiceLimitedConnectionsTest : public ::testing::Test {

public:
    wrench::WorkflowFile *files[10];
    wrench::ComputeService *compute_service = nullptr;
    wrench::StorageService *storage_service_wms = nullptr;
    wrench::StorageService *storage_service_1 = nullptr;
    wrench::StorageService *storage_service_2 = nullptr;

    void do_ConcurrencyFileCopies_test();


protected:
    SimpleStorageServiceLimitedConnectionsTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create the files
      for (size_t i=0; i < 10; i++) {
        files[i] = workflow->addFile("file_"+std::to_string(i), FILE_SIZE);
      }

      // Create a 3-host platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <AS id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\"/> "
              "       <host id=\"Host2\" speed=\"1f\"/> "
              "       <host id=\"WMSHost\" speed=\"1f\"/> "
              "       <link id=\"link1\" bandwidth=\"10MBps\" latency=\"100us\"/>"
              "       <link id=\"link2\" bandwidth=\"10MBps\" latency=\"100us\"/>"
              "       <route src=\"WMSHost\" dst=\"Host1\">"
              "         <link_ctn id=\"link1\"/>"
              "       </route>"
              "       <route src=\"WMSHost\" dst=\"Host2\">"
              "         <link_ctn id=\"link2\"/>"
              "       </route>"
              "   </AS> "
              "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::string platform_file_path = "/tmp/platform.xml";
    wrench::Workflow *workflow;
};


/**********************************************************************/
/**  CONCURRENT FILE COPIES TEST                                     **/
/**********************************************************************/

class SimpleStorageServiceConcurrencyFileCopiesLimitedConnectionsTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceConcurrencyFileCopiesLimitedConnectionsTestWMS(SimpleStorageServiceLimitedConnectionsTest *test,
                                                                       wrench::Workflow *workflow,
                                                                       std::unique_ptr<wrench::Scheduler> scheduler,
                                                                       const std::set<wrench::ComputeService *> &compute_services,
                                                                       const std::set<wrench::StorageService *> &storage_services,
                                                                       const std::string &hostname) :
            wrench::WMS(workflow, std::move(scheduler), compute_services, storage_services, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceLimitedConnectionsTest *test;

    int main() {


      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Get a file registry
      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Reading
      for (auto dst_storage_service : {this->test->storage_service_1, this->test->storage_service_2}) {

        // Initiate 10 asynchronous file copies to storage_service_1 (unlimited)
        double start = this->simulation->getCurrentSimulatedDate();
        for (int i = 0; i < 10; i++) {
          std::cerr << "I = " << i << "\n";
          data_movement_manager->initiateAsynchronousFileCopy(this->test->files[i],
                                                              this->test->storage_service_wms,
                                                              dst_storage_service);
        }

        double completion_dates[10];
        for (int i = 0; i < 10; i++) {
          std::unique_ptr<wrench::WorkflowExecutionEvent> event1 = workflow->waitForNextExecutionEvent();
          if (event1->type != wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION) {
            throw std::runtime_error("Unexpected Workflow Execution Event " + std::to_string(event1->type));
          }
          completion_dates[i] = this->simulation->getCurrentSimulatedDate();
        }

        for (int i = 0; i < 10; i++) {
          std::cerr << "COMPLETION DATE #" << i << ":  " << completion_dates[i] << "\n";
        }

        // Check results for the unlimited storage service
        if (dst_storage_service == this->test->storage_service_1) {
          double baseline_elapsed = completion_dates[0] - start;
          std::cerr << "BASELINE ELAPSED " << baseline_elapsed << "\n";
          for (int i=1; i < 10; i++) {
            if (fabs(baseline_elapsed - (completion_dates[i] - start)) > 0.1) {
              throw std::runtime_error("Incoherent transfer elapsed times for the unlimited storage service");
            }
          }
        }

        // Check results for the limited storage service
        if (dst_storage_service == this->test->storage_service_2) {
          bool success = true;

          if ((fabs(completion_dates[0] - completion_dates[1]) > 0.01) ||
              (fabs(completion_dates[2] - completion_dates[1]) > 0.01) ||
              (fabs(completion_dates[4] - completion_dates[3]) > 0.01) ||
              (fabs(completion_dates[5] - completion_dates[4]) > 0.01) ||
              (fabs(completion_dates[7] - completion_dates[6]) > 0.01) ||
              (fabs(completion_dates[8] - completion_dates[7]) > 0.01)) {
            success = false;
          }

          if (not success) {
            throw std::runtime_error("Incoherent transfer elapsed times for the limited storage service");
          }
        }
      }


      // Terminate
      this->shutdownAllServices();
      return 0;
    }
};

TEST_F(SimpleStorageServiceLimitedConnectionsTest, ConcurrencyFileCopies) {
  DO_TEST_WITH_FORK(do_ConcurrencyFileCopies_test);
}

void SimpleStorageServiceLimitedConnectionsTest::do_ConcurrencyFileCopies_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("performance_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a (unused) Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService("WMSHost", true, true,
                                                               {std::make_tuple("WMSHost", wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                               nullptr, {}))));

  // Create a Local storage service with unlimited connections
  EXPECT_NO_THROW(storage_service_wms = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService("WMSHost", STORAGE_SIZE, ULONG_MAX))));

  // Create a Storage service with unlimited connections
  EXPECT_NO_THROW(storage_service_1 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService("Host1", STORAGE_SIZE, ULONG_MAX))));

  // Create a Storage Service limited to 3 connections
  EXPECT_NO_THROW(storage_service_2 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService("Host2", STORAGE_SIZE, 3))));

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->add(
          std::unique_ptr<wrench::WMS>(new SimpleStorageServiceConcurrencyFileCopiesLimitedConnectionsTestWMS(
                  this, workflow, std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), {compute_service}, {storage_service_wms, storage_service_1, storage_service_2},
                          "WMSHost"))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService("WMSHost"));

  simulation->setFileRegistryService(std::move(file_registry_service));

  // Staging all files on the WMS storage service
  for (int i=0; i < 10; i++) {
    EXPECT_NO_THROW(simulation->stageFile(files[i], storage_service_wms));
  }

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}