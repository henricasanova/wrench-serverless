/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "NoopScheduler.h"

#include "TestWithFork.h"

#define EPSILON 0.05

class MulticoreComputeServiceTestStandardJobs : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::StorageService *storage_service = nullptr;
    wrench::WorkflowFile *output_file1;
    wrench::WorkflowFile *output_file2;
    wrench::WorkflowFile *output_file3;
    wrench::WorkflowFile *output_file4;
    wrench::WorkflowTask *task1;
    wrench::WorkflowTask *task2;
    wrench::WorkflowTask *task3;
    wrench::WorkflowTask *task4;
    wrench::WorkflowTask *task5;
    wrench::WorkflowTask *task6;

    wrench::ComputeService *compute_service = nullptr;

    void do_UnsupportedStandardJobs_test();

    void do_TwoSingleCoreTasks_test();

    void do_TwoDualCoreTasksCase1_test();

    void do_TwoDualCoreTasksCase2_test();

    void do_JobTermination_test();

    void do_NonSubmittedJobTermination_test();

    void do_CompletedJobTermination_test();

    void do_ShutdownComputeServiceWhileJobIsRunning_test();

    void do_ShutdownStorageServiceBeforeJobIsSubmitted_test();


protected:
    MulticoreComputeServiceTestStandardJobs() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create the files
      input_file = workflow->addFile("input_file", 10.0);
      output_file1 = workflow->addFile("output_file1", 10.0);
      output_file2 = workflow->addFile("output_file2", 10.0);
      output_file3 = workflow->addFile("output_file3", 10.0);
      output_file4 = workflow->addFile("output_file4", 10.0);

      // Create the tasks
      task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 1.0);
      task2 = workflow->addTask("task_2_10s_1core", 10.0, 1, 1, 1.0);
      task3 = workflow->addTask("task_3_10s_2cores", 10.0, 2, 2, 1.0);
      task4 = workflow->addTask("task_4_10s_2cores", 10.0, 2, 2, 1.0);
      task5 = workflow->addTask("task_5_30s_1_to_3_cores", 30.0, 1, 3, 1.0);
      task6 = workflow->addTask("task_6_10s_1_to_2_cores", 12.0, 1, 2, 1.0);

      // Add file-task dependencies
      task1->addInputFile(input_file);
      task2->addInputFile(input_file);
      task3->addInputFile(input_file);
      task4->addInputFile(input_file);
      task5->addInputFile(input_file);
      task6->addInputFile(input_file);


      task1->addOutputFile(output_file1);
      task2->addOutputFile(output_file2);
      task3->addOutputFile(output_file3);
      task4->addOutputFile(output_file4);
      task5->addOutputFile(output_file3);
      task6->addOutputFile(output_file4);


      // Create a one-host dual-core platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <AS id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"/> "
              "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\"/> "
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
/**  UNSUPPORTED JOB TYPE TEST                                       **/
/**********************************************************************/

class MulticoreComputeServiceUnsupportedJobTypeTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceUnsupportedJobTypeTestWMS(MulticoreComputeServiceTestStandardJobs *test,
                                                     wrench::Workflow *workflow,
                                                     std::unique_ptr<wrench::Scheduler> scheduler,
                                                     std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestStandardJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2}, {}, {},
                                                                         {}, {});

      // Submit the 2-task job for execution
      bool success = true;
      try {
        job_manager->submitJob(two_task_job, this->test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        if (e.getCause()->getCauseType() != wrench::FailureCause::JOB_TYPE_NOT_SUPPORTED) {
          throw std::runtime_error("Didn't get the expected exception");
        }
        success = false;
      }
      if (success) {
        throw std::runtime_error(
                "Should not be able to submit a pilot job to a compute service that does not support them");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestStandardJobs, UnsupportedStandardJobs) {
  DO_TEST_WITH_FORK(do_UnsupportedStandardJobs_test);
}

void MulticoreComputeServiceTestStandardJobs::do_UnsupportedStandardJobs_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceUnsupportedJobTypeTestWMS(this, workflow,
                                                                                            std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, false, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));


  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
}

/**********************************************************************/
/**  TWO SINGLE CORE TASKS TEST                                      **/
/**********************************************************************/

class MulticoreComputeServiceTwoSingleCoreTasksTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceTwoSingleCoreTasksTestWMS(MulticoreComputeServiceTestStandardJobs *test,
                                                     wrench::Workflow *workflow,
                                                     std::unique_ptr<wrench::Scheduler> scheduler,
                                                     std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestStandardJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2}, {}, {},
                                                                         {}, {});

      // Submit the 2-task job for execution
      job_manager->submitJob(two_task_job, this->test->compute_service);

      // Wait for a workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = workflow->waitForNextExecutionEvent();
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

      // Check completion states and times
      if ((this->test->task1->getState() != wrench::WorkflowTask::COMPLETED) ||
          (this->test->task2->getState() != wrench::WorkflowTask::COMPLETED)) {
        throw std::runtime_error("Unexpected task states");
      }

      double task1_end_date = this->test->task1->getEndDate();
      double task2_end_date = this->test->task2->getEndDate();
      double delta = fabs(task1_end_date - task2_end_date);
      if (delta > 0.1) {
        throw std::runtime_error("Task completion times should be about 0.0 seconds apart but they are " +
                                 std::to_string(delta) + " apart.");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestStandardJobs, TwoSingleCoreTasks) {
  DO_TEST_WITH_FORK(do_TwoSingleCoreTasks_test);
}

void MulticoreComputeServiceTestStandardJobs::do_TwoSingleCoreTasks_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceTwoSingleCoreTasksTestWMS(this, workflow,
                                                                                            std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));
  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));


  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
}

/**********************************************************************/
/**  TWO DUAL-CORE TASKS TEST #1                                     **/
/**********************************************************************/

class MulticoreComputeServiceTwoDualCoreTasksCase1TestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceTwoDualCoreTasksCase1TestWMS(MulticoreComputeServiceTestStandardJobs *test,
                                                        wrench::Workflow *workflow,
                                                        std::unique_ptr<wrench::Scheduler> scheduler,
                                                        std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestStandardJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task3, this->test->task4}, {}, {},
                                                                         {}, {});

      // Submit the 2-task job for execution
      job_manager->submitJob(two_task_job, this->test->compute_service);

      // Wait for the job completion
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = workflow->waitForNextExecutionEvent();
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

      // Check completion states and times
      if ((this->test->task3->getState() != wrench::WorkflowTask::COMPLETED) ||
          (this->test->task4->getState() != wrench::WorkflowTask::COMPLETED)) {
        throw std::runtime_error("Unexpected task states");
      }

      double task3_end_date = this->test->task3->getEndDate();
      double task4_end_date = this->test->task4->getEndDate();
      double delta = fabs(task3_end_date - task4_end_date);
      if ((delta < 5.0) || (delta > 5.0 + EPSILON)) {
        throw std::runtime_error("Unexpected task completion times " + std::to_string(task3_end_date) + " and " +
                                 std::to_string(task4_end_date) + ".");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};


TEST_F(MulticoreComputeServiceTestStandardJobs, TwoDualCoreTasksCase1) {
  DO_TEST_WITH_FORK(do_TwoDualCoreTasksCase1_test);
}

void MulticoreComputeServiceTestStandardJobs::do_TwoDualCoreTasksCase1_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceTwoDualCoreTasksCase1TestWMS(this, workflow,
                                                                                               std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));


  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
}


/**********************************************************************/
/**  TWO DUAL-CORE TASKS TEST #2                                     **/
/**********************************************************************/

class MulticoreComputeServiceTwoDualCoreTasksCase2TestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceTwoDualCoreTasksCase2TestWMS(MulticoreComputeServiceTestStandardJobs *test,
                                                        wrench::Workflow *workflow,
                                                        std::unique_ptr<wrench::Scheduler> scheduler,
                                                        std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestStandardJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task5, this->test->task6}, {}, {},
                                                                         {}, {});

      // Submit the 2-task job for execution
      job_manager->submitJob(two_task_job, this->test->compute_service);

      // Wait for the job completion
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = workflow->waitForNextExecutionEvent();
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

      // Check completion states and times
      if ((this->test->task5->getState() != wrench::WorkflowTask::COMPLETED) ||
          (this->test->task6->getState() != wrench::WorkflowTask::COMPLETED)) {
        throw std::runtime_error("Unexpected task states");
      }

      double task5_end_date = this->test->task5->getEndDate();
      double task6_end_date = this->test->task6->getEndDate();


      if ((task5_end_date < 10.0) || (task5_end_date > 10.0 + EPSILON)) {
        throw std::runtime_error("Unexpected task5 end date " + std::to_string(task5_end_date) + " (should be 10.0)");
      }

      if ((task6_end_date < 12.0) || (task6_end_date > 12.0 + EPSILON)) {
        throw std::runtime_error("Unexpected task6 end date " + std::to_string(task6_end_date) + " (should be 12.0)");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};


TEST_F(MulticoreComputeServiceTestStandardJobs, TwoDualCoreTasksCase2) {
  DO_TEST_WITH_FORK(do_TwoDualCoreTasksCase2_test);
}

void MulticoreComputeServiceTestStandardJobs::do_TwoDualCoreTasksCase2_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "QuadCoreHost";

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceTwoDualCoreTasksCase2TestWMS(this, workflow,
                                                                                               std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));


  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
}


/**********************************************************************/
/**  JOB TERMINATION TEST                                            **/
/**********************************************************************/

class MulticoreComputeServiceJobTerminationTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceJobTerminationTestWMS(MulticoreComputeServiceTestStandardJobs *test,
                                                 wrench::Workflow *workflow,
                                                 std::unique_ptr<wrench::Scheduler> scheduler,
                                                 std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestStandardJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2}, {}, {},
                                                                         {}, {});

      // Submit the 2-task job for execution
      job_manager->submitJob(two_task_job, this->test->compute_service);

      // Immediately terminate it
      try {
        job_manager->terminateJob(two_task_job);
      } catch (std::exception &e) {
        throw std::runtime_error("Unexpected exception while terminating job: " + std::string(e.what()));
      }

      // Check that the job's state has been updated
      if (two_task_job->getState() != wrench::StandardJob::TERMINATED) {
        throw std::runtime_error("Terminated Standard Job is not in TERMINATED state");
      }

      // Check that task states make sense
      if ((this->test->task1->getState() != wrench::WorkflowTask::READY) ||
          (this->test->task2->getState() != wrench::WorkflowTask::READY)) {
        throw std::runtime_error("Tasks in a FAILED job should be in the READY state");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestStandardJobs, JobTermination) {
  DO_TEST_WITH_FORK(do_JobTermination_test);
}

void MulticoreComputeServiceTestStandardJobs::do_JobTermination_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceJobTerminationTestWMS(this, workflow,
                                                                                        std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));


  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  // Check completion states and times
  if ((this->task1->getState() != wrench::WorkflowTask::READY) ||
      (this->task2->getState() != wrench::WorkflowTask::READY)) {
    throw std::runtime_error("Unexpected task states: [" + this->task1->getId() + ": " +
                             wrench::WorkflowTask::stateToString(this->task1->getState()) + ", " +
                             this->task2->getId() + ": " +
                             wrench::WorkflowTask::stateToString(this->task2->getState()) + "]");
  }

  // Check failure counts: Terminations DO NOT count as failures
  if ((this->task1->getFailureCount() != 0) ||
      (this->task2->getFailureCount() != 0)) {
    throw std::runtime_error("Unexpected task failure counts: [" + this->task1->getId() + ": " +
                             std::to_string(this->task1->getFailureCount()) + ", " +
                             this->task2->getId() + ": " + std::to_string(this->task2->getFailureCount()) + "]");
  }


  delete simulation;
}


/**********************************************************************/
/**  NOT SUBMITTED JOB TERMINATION TEST                              **/
/**********************************************************************/

class MulticoreComputeServiceNonSubmittedJobTerminationTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceNonSubmittedJobTerminationTestWMS(MulticoreComputeServiceTestStandardJobs *test,
                                                             wrench::Workflow *workflow,
                                                             std::unique_ptr<wrench::Scheduler> scheduler,
                                                             std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestStandardJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2}, {}, {},
                                                                         {}, {});

      // Try to terminate it now (which is stupid)
      bool success = true;
      try {
        job_manager->terminateJob(two_task_job);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        if (e.getCause()->getCauseType() != wrench::FailureCause::JOB_CANNOT_BE_TERMINATED) {
          throw std::runtime_error(
                  "Got an exception, as expected, but it does not have the correct failure cause type");
        }
        wrench::JobCannotBeTerminated *real_cause = (wrench::JobCannotBeTerminated *) e.getCause().get();
        if (real_cause->getJob() != two_task_job) {
          throw std::runtime_error(
                  "Got the expected exception and failure cause, but the failure cause does not point to the right job");
        }
      }
      if (success) {
        throw std::runtime_error("Trying to terminate a non-submitted job should have raised an exception!");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestStandardJobs, NonSubmittedJobTermination) {
  DO_TEST_WITH_FORK(do_NonSubmittedJobTermination_test);
}

void MulticoreComputeServiceTestStandardJobs::do_NonSubmittedJobTermination_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceNonSubmittedJobTerminationTestWMS(this, workflow,
                                                                                                    std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));


  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  // Check completion states and times
  if ((this->task1->getState() != wrench::WorkflowTask::READY) ||
      (this->task2->getState() != wrench::WorkflowTask::READY)) {
    throw std::runtime_error("Unexpected task states: [" + this->task1->getId() + ": " +
                             wrench::WorkflowTask::stateToString(this->task1->getState()) + ", " +
                             this->task2->getId() + ": " +
                             wrench::WorkflowTask::stateToString(this->task2->getState()) + "]");
  }

  // Check failure counts: Terminations DO NOT count as failures
  if ((this->task1->getFailureCount() != 0) ||
      (this->task2->getFailureCount() != 0)) {
    throw std::runtime_error("Unexpected task failure counts: [" + this->task1->getId() + ": " +
                             std::to_string(this->task1->getFailureCount()) + ", " +
                             this->task2->getId() + ": " + std::to_string(this->task2->getFailureCount()) + "]");
  }


  delete simulation;
}


/**********************************************************************/
/**  COMPLETED JOB TERMINATION TEST                                  **/
/**********************************************************************/

class MulticoreComputeServiceCompletedJobTerminationTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceCompletedJobTerminationTestWMS(MulticoreComputeServiceTestStandardJobs *test,
                                                          wrench::Workflow *workflow,
                                                          std::unique_ptr<wrench::Scheduler> scheduler,
                                                          std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestStandardJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2}, {}, {},
                                                                         {}, {});

      // Submit the 2-task job for execution
      job_manager->submitJob(two_task_job, this->test->compute_service);

      // Wait for the job completion
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = workflow->waitForNextExecutionEvent();
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

      // Try to terminate it now (which is stupid)
      bool success = true;
      try {
        job_manager->terminateJob(two_task_job);
      } catch (std::exception &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Trying to terminate a non-submitted job should have raised an exception!");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestStandardJobs, CompletedJobTermination) {
  DO_TEST_WITH_FORK(do_CompletedJobTermination_test);
}

void MulticoreComputeServiceTestStandardJobs::do_CompletedJobTermination_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceCompletedJobTerminationTestWMS(this, workflow,
                                                                                                 std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));


  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  // Check completion states and times
  if ((this->task1->getState() != wrench::WorkflowTask::COMPLETED) ||
      (this->task2->getState() != wrench::WorkflowTask::COMPLETED)) {
    throw std::runtime_error("Unexpected task states: [" + this->task1->getId() + ": " +
                             wrench::WorkflowTask::stateToString(this->task1->getState()) + ", " +
                             this->task2->getId() + ": " +
                             wrench::WorkflowTask::stateToString(this->task2->getState()) + "]");
  }

  // Check failure counts: Terminations DO NOT count as failures
  if ((this->task1->getFailureCount() != 0) ||
      (this->task2->getFailureCount() != 0)) {
    throw std::runtime_error("Unexpected task failure counts: [" + this->task1->getId() + ": " +
                             std::to_string(this->task1->getFailureCount()) + ", " +
                             this->task2->getId() + ": " + std::to_string(this->task2->getFailureCount()) + "]");
  }


  delete simulation;
}


/**********************************************************************/
/**  COMPUTE SERVICE SHUTDOWN WHILE JOB IS RUNNING TEST              **/
/**********************************************************************/

class MulticoreComputeServiceShutdownComputeServiceWhileJobIsRunningTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceShutdownComputeServiceWhileJobIsRunningTestWMS(MulticoreComputeServiceTestStandardJobs *test,
                                                                          wrench::Workflow *workflow,
                                                                          std::unique_ptr<wrench::Scheduler> scheduler,
                                                                          std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestStandardJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2}, {}, {},
                                                                         {}, {});

      // Submit the 2-task job for execution
      job_manager->submitJob(two_task_job, this->test->compute_service);

      // Shutdown all compute services
      this->simulation->shutdownAllComputeServices();

      // Wait for the job failure notification
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
      }
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
          if (event->failure_cause->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
            throw std::runtime_error("Got a job failure event, but the failure cause seems wrong");
          }
          wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) (event->failure_cause.get());
          if (real_cause->getService() != this->test->compute_service) {
            std::runtime_error(
                    "Got the correct failure even, a correct cause type, but the cause points to the wrong service");
          }
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
        }
      }

      // Terminate
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestStandardJobs, ShutdownComputeServiceWhileJobIsRunning) {
  DO_TEST_WITH_FORK(do_ShutdownComputeServiceWhileJobIsRunning_test);
}

void MulticoreComputeServiceTestStandardJobs::do_ShutdownComputeServiceWhileJobIsRunning_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(
                  new MulticoreComputeServiceShutdownComputeServiceWhileJobIsRunningTestWMS(this, workflow,
                                                                                            std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));


  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  // Check completion states and times
  if ((this->task1->getState() != wrench::WorkflowTask::READY) ||
      (this->task2->getState() != wrench::WorkflowTask::READY)) {
    throw std::runtime_error("Unexpected task states: [" + this->task1->getId() + ": " +
                             wrench::WorkflowTask::stateToString(this->task1->getState()) + ", " +
                             this->task2->getId() + ": " +
                             wrench::WorkflowTask::stateToString(this->task2->getState()) + "]");
  }

  // Check failure counts: Terminations DO NOT count as failures
  if ((this->task1->getFailureCount() != 1) ||
      (this->task2->getFailureCount() != 1)) {
    throw std::runtime_error("Unexpected task failure counts: [" + this->task1->getId() + ": " +
                             std::to_string(this->task1->getFailureCount()) + ", " +
                             this->task2->getId() + ": " + std::to_string(this->task2->getFailureCount()) + "]");
  }

  delete simulation;
}


/**********************************************************************/
/**  STORAGE SERVICE SHUTDOWN BEFORE JOB IS SUBMITTED TEST           **/
/**********************************************************************/

class MulticoreComputeServiceShutdownStorageServiceBeforeJobIsSubmittedTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceShutdownStorageServiceBeforeJobIsSubmittedTestWMS(
            MulticoreComputeServiceTestStandardJobs *test,
            wrench::Workflow *workflow,
            std::unique_ptr<wrench::Scheduler> scheduler,
            std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestStandardJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2}, {}, {},
                                                                         {}, {});

      // Shutdown all storage services
      this->simulation->shutdownAllStorageServices();

      // Submit the 2-task job for execution
      job_manager->submitJob(two_task_job, this->test->compute_service);

      // Wait for the job failure notification
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
      }
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
          if (event->failure_cause->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
            std::runtime_error("Got the correct failure event, but the cause seems wrong");
          }
          wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) (event->failure_cause.get());
          if (real_cause->getService() != this->test->storage_service) {
            std::runtime_error(
                    "Got the correct failure even, a correct cause type, but the cause points to the wrong service");
          }
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
        }
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestStandardJobs, ShutdownStorageServiceBeforeJobIsSubmitted) {
  DO_TEST_WITH_FORK(do_ShutdownStorageServiceBeforeJobIsSubmitted_test);
}

void MulticoreComputeServiceTestStandardJobs::do_ShutdownStorageServiceBeforeJobIsSubmitted_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(
                  new MulticoreComputeServiceShutdownStorageServiceBeforeJobIsSubmittedTestWMS(this, workflow,
                                                                                               std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));


  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  // Check completion states and times
  if ((this->task1->getState() != wrench::WorkflowTask::READY) ||
      (this->task2->getState() != wrench::WorkflowTask::READY)) {
    throw std::runtime_error("Unexpected task states: [" + this->task1->getId() + ": " +
                             wrench::WorkflowTask::stateToString(this->task1->getState()) + ", " +
                             this->task2->getId() + ": " +
                             wrench::WorkflowTask::stateToString(this->task2->getState()) + "]");
  }

  // Check failure counts: Terminations DO NOT count as failures
  if ((this->task1->getFailureCount() != 1) ||
      (this->task2->getFailureCount() != 1)) {
    throw std::runtime_error("Unexpected task failure counts: [" + this->task1->getId() + ": " +
                             std::to_string(this->task1->getFailureCount()) + ", " +
                             this->task2->getId() + ": " + std::to_string(this->task2->getFailureCount()) + "]");
  }

  delete simulation;
}
