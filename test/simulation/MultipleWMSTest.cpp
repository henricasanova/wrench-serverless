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
#include <numeric>

#include "NoopScheduler.h"
#include "TestWithFork.h"

class MultipleWMSTest : public ::testing::Test {

public:
    wrench::ComputeService *compute_service = nullptr;
    wrench::StorageService *storage_service = nullptr;

    void do_deferredWMSStartOneWMS_test();

    void do_deferredWMSStartTwoWMS_test();

protected:
    MultipleWMSTest() {
      // Create a platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <AS id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"/> "
              "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\"/> "
              "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
              "       <route src=\"DualCoreHost\" dst=\"QuadCoreHost\"> <link_ctn id=\"1\"/> </route>"
              "   </AS> "
              "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);
    }

    wrench::Workflow *createWorkflow() {
      wrench::Workflow *workflow;
      wrench::WorkflowFile *input_file;
      wrench::WorkflowFile *output_file1;
      wrench::WorkflowFile *output_file2;
      wrench::WorkflowTask *task1;
      wrench::WorkflowTask *task2;

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create the files
      input_file = workflow->addFile("input_file", 10.0);
      output_file1 = workflow->addFile("output_file1", 10.0);
      output_file2 = workflow->addFile("output_file2", 10.0);

      // Create the tasks
      task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 1.0);
      task2 = workflow->addTask("task_2_10s_1core", 10.0, 1, 1, 1.0);

      // Add file-task dependencies
      task1->addInputFile(input_file);
      task2->addInputFile(input_file);

      task1->addOutputFile(output_file1);
      task2->addOutputFile(output_file2);

      return workflow;
    }

    std::string platform_file_path = "/tmp/platform.xml";
};

/**********************************************************************/
/**  DEFERRED WMS START TIME WITH ONE WMS ON ONE HOST                **/
/**********************************************************************/

class DeferredWMSStartTestWMS : public wrench::WMS {

public:
    DeferredWMSStartTestWMS(MultipleWMSTest *test,
                            wrench::Workflow *workflow,
                            std::unique_ptr<wrench::Scheduler> scheduler,
                            const std::set<wrench::ComputeService *> &compute_services,
                            const std::set<wrench::StorageService *> &storage_services,
                            std::string &hostname, double start_time) :
            wrench::WMS(workflow, std::move(scheduler), compute_services, storage_services, hostname, "test",
                        start_time) {
      this->test = test;
    }

private:

    MultipleWMSTest *test;

    int main() {
      // check for deferred start
      checkDeferredStart();

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      // Create a file registry service
      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob(this->workflow->getTasks(), {}, {},
                                                                         {}, {});

      // Submit the 2-task job for execution
      try {
        auto cs = (wrench::CloudService *) *this->getRunningComputeServices().begin();
        std::string execution_host = cs->getExecutionHosts()[0];
        cs->createVM(execution_host, "vm_" + execution_host, 2);
        job_manager->submitJob(two_task_job, this->test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      // Wait for a workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = this->workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
      }
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
          // success, do nothing for now
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
        }
      }

      // Terminate
      this->shutdownAllServices();
      return 0;
    }
};

TEST_F(MultipleWMSTest, DeferredWMSStartTestWMS) {
  DO_TEST_WITH_FORK(do_deferredWMSStartOneWMS_test);
  DO_TEST_WITH_FORK(do_deferredWMSStartTwoWMS_test);
}

void MultipleWMSTest::do_deferredWMSStartOneWMS_test() {
  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("multiple_wms_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Cloud Service
  std::vector<std::string> execution_hosts = {simulation->getHostnameList()[1]};
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::CloudService>(
                  new wrench::CloudService(hostname, true, false, execution_hosts, storage_service, {}))));

  // Create a WMS
  wrench::Workflow *workflow = this->createWorkflow();
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->add(std::unique_ptr<wrench::WMS>(
          new DeferredWMSStartTestWMS(this, workflow, std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), {compute_service}, {storage_service}, hostname, 100))));

  // Create a file registry
  EXPECT_NO_THROW(simulation->setFileRegistryService(
          std::unique_ptr<wrench::FileRegistryService>(new wrench::FileRegistryService(hostname))));

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles(workflow->getInputFiles(), storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  // Simulation trace
  EXPECT_GT(simulation->getCurrentSimulatedDate(), 100);

  delete simulation;
  free(argv[0]);
  free(argv);
}

void MultipleWMSTest::do_deferredWMSStartTwoWMS_test() {
  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("multiple_wms_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Cloud Service
  std::vector<std::string> execution_hosts = {simulation->getHostnameList()[1]};
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::CloudService>(
                  new wrench::CloudService(hostname, true, false, execution_hosts, storage_service, {}))));

  // Create a WMS
  wrench::Workflow *workflow = this->createWorkflow();
  EXPECT_NO_THROW(simulation->add(std::unique_ptr<wrench::WMS>(
          new DeferredWMSStartTestWMS(this, workflow, std::unique_ptr<wrench::Scheduler>(
                  new NoopScheduler()), {compute_service}, {storage_service}, hostname, 100))));

  // Create a second WMS
  wrench::Workflow *workflow2 = this->createWorkflow();
  EXPECT_NO_THROW(simulation->add(std::unique_ptr<wrench::WMS>(
          new DeferredWMSStartTestWMS(this, workflow2, std::unique_ptr<wrench::Scheduler>(
                  new NoopScheduler()), {compute_service}, {storage_service}, hostname, 1000))));

  // Create a file registry
  EXPECT_NO_THROW(simulation->setFileRegistryService(
          std::unique_ptr<wrench::FileRegistryService>(new wrench::FileRegistryService(hostname))));

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles(workflow->getInputFiles(), storage_service));
  EXPECT_NO_THROW(simulation->stageFiles(workflow2->getInputFiles(), storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  // Simulation trace
  EXPECT_GT(simulation->getCurrentSimulatedDate(), 1000);

  delete simulation;
  free(argv[0]);
  free(argv);
}