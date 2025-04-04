/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>

#include <wrench/data_file/DataFile.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/workflow/WorkflowTask.h>
#include <wrench/workflow/Workflow.h>

class WorkflowTest : public ::testing::Test {
protected:
    ~WorkflowTest() {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    WorkflowTest() {

        workflow = wrench::Workflow::createWorkflow();

        // create simple diamond workflow
        t1 = workflow->addTask("task1-test-01", 1, 1, 1, 0);
        t2 = workflow->addTask("task1-test-02", 1, 1, 1, 0);
        t3 = workflow->addTask("task1-test-03", 1, 1, 1, 0);
        t4 = workflow->addTask("task1-test-04", 1, 1, 1, 0);

        t2->setClusterID("cluster-01");
        t3->setClusterID("cluster-01");

        workflow->addControlDependency(t1, t2);
        workflow->addControlDependency(t1, t3);
        workflow->addControlDependency(t2, t4);
        workflow->addControlDependency(t3, t4);

        // Add a cycle-producing dependency
        try {
            workflow->addControlDependency(t2, t1);
            throw std::runtime_error("Creating a dependency cycle in workflow should throw");
        } catch (std::runtime_error &ignore) {}

        f1 = wrench::Simulation::addFile("file-01", 1);
        f2 = wrench::Simulation::addFile("file-02", 1);
        f3 = wrench::Simulation::addFile("file-03", 1);
        f4 = wrench::Simulation::addFile("file-04", 1);
        f5 = wrench::Simulation::addFile("file-05", 1);

        t1->addInputFile(f1);
        t2->addInputFile(f2);
        t1->addOutputFile(f2);
        t2->addOutputFile(f3);
        t3->addInputFile(f2);
        t3->addOutputFile(f4);
        t4->addInputFile(f3);
        t4->addInputFile(f4);
        t4->addOutputFile(f5);


        // Coverage
        auto tasks = workflow->getTasksThatInput(f2);
        if ((tasks.find(t2) == tasks.end()) or (tasks.find(t3) == tasks.end())) {
            throw std::runtime_error("getTasksThatInput() doesn't generate the same output");
        }
        t1->getPriority();
        try {
            t1->updateStartDate(666.6);
            throw std::runtime_error("Should not be able to call WorkflowTask::updateStartDate()");
        } catch (std::runtime_error &ignore) {}
        try {
            t1->setTerminationDate(666.6);
            throw std::runtime_error("Should not be able to call WorkflowTask::setTerminationDate()");
        } catch (std::runtime_error &ignore) {}
    }

    // data members
    std::shared_ptr<wrench::Workflow> workflow;
    std::shared_ptr<wrench::WorkflowTask> t1, t2, t3, t4;
    std::shared_ptr<wrench::DataFile> f1, f2, f3, f4, f5;
};

TEST_F(WorkflowTest, WorkflowStructure) {
    ASSERT_EQ(4, workflow->getNumberOfTasks());

    // testing number of task1's parents
    ASSERT_EQ(0, workflow->getTaskParents(t1).size());
    ASSERT_EQ(1, workflow->getTaskParents(t2).size());
    ASSERT_EQ(1, workflow->getTaskParents(t3).size());
    ASSERT_EQ(2, workflow->getTaskParents(t4).size());

    // testing number of task1's children
    ASSERT_EQ(2, workflow->getTaskChildren(t1).size());
    ASSERT_EQ(1, workflow->getTaskChildren(t2).size());
    ASSERT_EQ(1, workflow->getTaskChildren(t3).size());
    ASSERT_EQ(0, workflow->getTaskChildren(t4).size());

    // testing top-levels
    ASSERT_EQ(0, t1->getTopLevel());
    ASSERT_EQ(1, t2->getTopLevel());
    ASSERT_EQ(1, t3->getTopLevel());
    ASSERT_EQ(2, t4->getTopLevel());

    // testing paths
    ASSERT_EQ(true, workflow->pathExists(t1, t3));
    ASSERT_EQ(false, workflow->pathExists(t3, t2));

    ASSERT_EQ(3, workflow->getNumLevels());

    //  Test task1 "getters"
    auto task_map = workflow->getTaskMap();
    ASSERT_EQ(4, task_map.size());
    auto tasks = workflow->getTasks();
    ASSERT_EQ(4, tasks.size());
    auto etask_map = workflow->getEntryTaskMap();
    ASSERT_EQ(1, etask_map.size());
    auto etasks = workflow->getEntryTasks();
    ASSERT_EQ(1, etasks.size());
    auto xtask_map = workflow->getExitTaskMap();
    ASSERT_EQ(1, xtask_map.size());
    auto xtasks = workflow->getExitTasks();
    ASSERT_EQ(1, xtasks.size());

    // Test file "getters"
    auto file_map = workflow->getFileMap();
    ASSERT_EQ(5, file_map.size());
    auto files = workflow->getFileMap();
    ASSERT_EQ(5, files.size());
    auto ifile_map = workflow->getInputFileMap();
    ASSERT_EQ(1, ifile_map.size());
    auto ifiles = workflow->getInputFiles();
    ASSERT_EQ(1, ifiles.size());
    auto ofile_map = workflow->getOutputFileMap();
    ASSERT_EQ(1, ofile_map.size());
    auto ofiles = workflow->getOutputFiles();
    ASSERT_EQ(1, ofiles.size());

    ASSERT_THROW(workflow->getTaskNumberOfChildren(nullptr), std::invalid_argument);
    ASSERT_THROW(workflow->getTaskNumberOfParents(nullptr), std::invalid_argument);
    // Get tasks with a given top-level
    std::vector<std::shared_ptr<wrench::WorkflowTask>> top_level_equal_to_1_or_2;
    top_level_equal_to_1_or_2 = workflow->getTasksInTopLevelRange(1, 2);
    ASSERT_EQ(std::find(top_level_equal_to_1_or_2.begin(), top_level_equal_to_1_or_2.end(), t1),
              top_level_equal_to_1_or_2.end());
    ASSERT_NE(std::find(top_level_equal_to_1_or_2.begin(), top_level_equal_to_1_or_2.end(), t2),
              top_level_equal_to_1_or_2.end());
    ASSERT_NE(std::find(top_level_equal_to_1_or_2.begin(), top_level_equal_to_1_or_2.end(), t3),
              top_level_equal_to_1_or_2.end());
    ASSERT_NE(std::find(top_level_equal_to_1_or_2.begin(), top_level_equal_to_1_or_2.end(), t4),
              top_level_equal_to_1_or_2.end());


    // Get Entry tasks and check they all are in the top level, as expected
    auto entry_tasks = workflow->getEntryTaskMap();
    auto top_level = workflow->getTasksInTopLevelRange(0, 0);
    for (auto const &t: top_level) {
        ASSERT_TRUE(entry_tasks.find(t->getID()) != entry_tasks.end());
    }
    // Being paranoid, check that they don't have parents
    for (auto const &t: entry_tasks) {
        ASSERT_EQ(t.second->getNumberOfParents(), 0);
    }

    // Get Exit tasks
    auto exit_tasks = workflow->getExitTasks();
    ASSERT_EQ(exit_tasks.size(), 1);
    ASSERT_TRUE(*(exit_tasks.begin()) == t4);

    // remove tasks
    workflow->removeTask(t4);
    ASSERT_EQ(0, workflow->getTaskChildren(t3).size());
    ASSERT_EQ(0, workflow->getTaskChildren(t2).size());

    ASSERT_EQ(3, workflow->getTasks().size());

    workflow->removeTask(t1);
}

TEST_F(WorkflowTest, ControlDependency) {
    // testing null control dependencies
    ASSERT_THROW(workflow->addControlDependency(nullptr, nullptr), std::invalid_argument);
    ASSERT_THROW(workflow->addControlDependency(t1, nullptr), std::invalid_argument);
    ASSERT_THROW(workflow->addControlDependency(nullptr, t1), std::invalid_argument);
    workflow->addControlDependency(t2, t3);
    workflow->removeControlDependency(t2, t3);// removes something
    workflow->removeControlDependency(t1, t2);// nope (data depencency)
    ASSERT_EQ(true, workflow->pathExists(t1, t2));
    ASSERT_THROW(workflow->removeControlDependency(nullptr, t4), std::invalid_argument);
    ASSERT_THROW(workflow->removeControlDependency(t1, nullptr), std::invalid_argument);
    workflow->removeControlDependency(t1, t4);// nope (nothing)
    auto new_task = workflow->addTask("new_task", 1.0, 1, 1, 0);
    workflow->addControlDependency(t1, new_task);
    workflow->removeControlDependency(t1, new_task);

    auto new_task_top_level = new_task->getTopLevel();
    workflow->enableTopBottomLevelDynamicUpdates(false);
    workflow->addControlDependency(t1, new_task);
    workflow->enableTopBottomLevelDynamicUpdates(true);
    workflow->updateAllTopBottomLevels();
    ASSERT_EQ(new_task->getTopLevel(), new_task_top_level + 1);
}

void doTopBottomLevelsTest(bool dynamic_updates) {
    // Create a test workflow
    auto wf = wrench::Workflow::createWorkflow();
    auto t1 = wf->addTask("t1", 1.0, 1, 1, 0.0);
    auto t2 = wf->addTask("t2", 1.0, 1, 1, 0.0);
    auto t3 = wf->addTask("t3", 1.0, 1, 1, 0.0);
    auto t4 = wf->addTask("t4", 1.0, 1, 1, 0.0);
    auto t5 = wf->addTask("t5", 1.0, 1, 1, 0.0);

    // Add dependencies and check them
    if (not dynamic_updates) {
        wf->enableTopBottomLevelDynamicUpdates(false);
    }
    wf->addControlDependency(t1, t2);
    wf->addControlDependency(t1, t3);
    wf->addControlDependency(t3, t4);

    if (not dynamic_updates) {
        wf->updateAllTopBottomLevels();
    }
    //    std::cerr << "T1 TL=" << t1->getTopLevel() << "\n";
    //    std::cerr << "T1 BL=" << t1->getBottomLevel() << "\n";
    //    std::cerr << "T2 TL=" << t2->getTopLevel() << "\n";
    //    std::cerr << "T2 BL=" << t2->getBottomLevel() << "\n";
    //    std::cerr << "T3 TL=" << t3->getTopLevel() << "\n";
    //    std::cerr << "T3 BL=" << t3->getBottomLevel() << "\n";
    //    std::cerr << "T4 TL=" << t4->getTopLevel() << "\n";
    //    std::cerr << "T4 BL=" << t4->getBottomLevel() << "\n";

    ASSERT_TRUE(t1->getTopLevel() == 0 and t1->getBottomLevel() == 2);
    ASSERT_TRUE(t2->getTopLevel() == 1 and t2->getBottomLevel() == 0);
    ASSERT_TRUE(t3->getTopLevel() == 1 and t3->getBottomLevel() == 1);
    ASSERT_TRUE(t4->getTopLevel() == 2 and t4->getBottomLevel() == 0);
    ASSERT_TRUE(t5->getTopLevel() == 0 and t5->getBottomLevel() == 0);
    auto tl_range_1 = wf->getTasksInTopLevelRange(1, 1);
    ASSERT_TRUE(tl_range_1.size() == 2);
    ASSERT_TRUE(std::find(tl_range_1.begin(), tl_range_1.end(), t2) != tl_range_1.end());
    ASSERT_TRUE(std::find(tl_range_1.begin(), tl_range_1.end(), t3) != tl_range_1.end());
    auto bl_range_1 = wf->getTasksInBottomLevelRange(1, 1);
    ASSERT_TRUE(bl_range_1.size() == 1);
    ASSERT_TRUE(std::find(bl_range_1.begin(), bl_range_1.end(), t3) != bl_range_1.end());

    // Add/remove dependencies/tasks just for kicks
    wf->addControlDependency(t4, t5);
    wf->removeControlDependency(t1, t3);
    auto t0 = wf->addTask("t0", 1, 1, 1, 0);
    wf->addControlDependency(t0, t1);
    wf->addControlDependency(t0, t5);
    wf->addControlDependency(t1, t4);

    if (not dynamic_updates) {
        wf->updateAllTopBottomLevels();
    }

    ASSERT_TRUE(t0->getTopLevel() == 0 and t0->getBottomLevel() == 3);
    ASSERT_TRUE(t1->getTopLevel() == 1 and t1->getBottomLevel() == 2);
    ASSERT_TRUE(t2->getTopLevel() == 2 and t2->getBottomLevel() == 0);
    ASSERT_TRUE(t3->getTopLevel() == 0 and t3->getBottomLevel() == 2);
    ASSERT_TRUE(t4->getTopLevel() == 2 and t4->getBottomLevel() == 1);
    ASSERT_TRUE(t5->getTopLevel() == 3 and t5->getBottomLevel() == 0);

    ASSERT_EQ(wf->getTasksInTopLevelRange(1, 2).size(), 3);
    ASSERT_EQ(wf->getTasksInBottomLevelRange(0, 2).size(), 5);
}

TEST_F(WorkflowTest, TopBottomLevelsDynamic) {
    doTopBottomLevelsTest(true);
    doTopBottomLevelsTest(false);
}


TEST_F(WorkflowTest, WorkflowTaskThrow) {
    // testing invalid task1 creation
    ASSERT_THROW(workflow->addTask("task1-error", -100, 1, 1, 0), std::invalid_argument);
    ASSERT_THROW(workflow->addTask("task1-error", 100, 2, 1, 0), std::invalid_argument);

    // testing whether a task1 id exists
    ASSERT_THROW(workflow->getTaskByID("task1-test-00"), std::invalid_argument);
    ASSERT_TRUE(workflow->getTaskByID("task1-test-01")->getID() == t1->getID());

    // testing whether a task1 already exists (check via task1 id)
    ASSERT_THROW(workflow->addTask("task1-test-01", 1, 1, 1, 0), std::invalid_argument);

    // remove tasks
    ASSERT_THROW(workflow->removeTask(nullptr), std::invalid_argument);
    workflow->removeTask(t1);

    auto bogus_workflow = wrench::Workflow::createWorkflow();
    std::shared_ptr<wrench::WorkflowTask> bogus = bogus_workflow->addTask("bogus", 100.0, 1, 1, 0.0);
    ASSERT_THROW(bogus->setParallelModel(wrench::ParallelModel::AMDAHL(-2.0)), std::invalid_argument);
    ASSERT_THROW(bogus->setParallelModel(wrench::ParallelModel::AMDAHL(2.0)), std::invalid_argument);
    ASSERT_THROW(bogus->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(-2.0)), std::invalid_argument);
    ASSERT_THROW(bogus->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(2.0)), std::invalid_argument);
    ASSERT_THROW(workflow->removeTask(bogus), std::invalid_argument);
    bogus_workflow->removeTask(bogus);

    ASSERT_THROW(workflow->getTaskChildren(nullptr), std::invalid_argument);
    ASSERT_THROW(workflow->getTaskParents(nullptr), std::invalid_argument);

    bogus_workflow->clear();
    wrench::Simulation::removeAllFiles();

    //  ASSERT_THROW(workflow->updateTaskState(nullptr, wrench::WorkflowTask::State::FAILED), std::invalid_argument);
}

TEST_F(WorkflowTest, DataFile) {
    ASSERT_THROW(wrench::Simulation::addFile("file-01", 10), std::invalid_argument);

    ASSERT_THROW(wrench::Simulation::getFileByID("file-nonexist"), std::invalid_argument);
    ASSERT_EQ(wrench::Simulation::getFileByID("file-01")->getID(), "file-01");

    ASSERT_EQ(workflow->getInputFiles().size(), 1);
}

//
//TEST_F(WorkflowTest, UpdateTaskState) {
//  // testing update task1 state
//  workflow->updateTaskState(t1, wrench::WorkflowTask::State::READY);
//  ASSERT_EQ(1, workflow->getReadyTasks().size());
//
//  // testing
//  workflow->updateTaskState(t1, wrench::WorkflowTask::State::RUNNING);
//  workflow->updateTaskState(t1, wrench::WorkflowTask::State::COMPLETED);
//  workflow->updateTaskState(t2, wrench::WorkflowTask::State::READY);
//  workflow->updateTaskState(t3, wrench::WorkflowTask::State::READY);
//  ASSERT_EQ(1, workflow->getReadyTasks().size());
//  ASSERT_EQ(2, workflow->getReadyTasks()["cluster-01"].size());
//}

TEST_F(WorkflowTest, IsDone) {
    ASSERT_FALSE(workflow->isDone());

    for (const auto &task: workflow->getTasks()) {
        task->setInternalState(wrench::WorkflowTask::InternalState::TASK_COMPLETED);
        task->setState(wrench::WorkflowTask::State::COMPLETED);
    }

    ASSERT_TRUE(workflow->isDone());
}

TEST_F(WorkflowTest, SumFlops) {

    double sum_flops = 0;

    ASSERT_NO_THROW(sum_flops = wrench::Workflow::getSumFlops(workflow->getTasks()));
    ASSERT_EQ(sum_flops, 4.0);
}


class AllDependenciesWorkflowTest : public ::testing::Test {
protected:
    ~AllDependenciesWorkflowTest() override {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    AllDependenciesWorkflowTest() {
        workflow = wrench::Workflow::createWorkflow();

        // create simple diamond workflow
        t1 = workflow->addTask("task1-test-01", 1, 1, 1, 0);
        t2 = workflow->addTask("task1-test-02", 1, 1, 1, 0);
        t3 = workflow->addTask("task1-test-03", 1, 1, 1, 0);
        t4 = workflow->addTask("task1-test-04", 1, 1, 1, 0);

        workflow->addControlDependency(t1, t1, true);// coverage
        workflow->addControlDependency(t1, t2, true);
        workflow->addControlDependency(t1, t3, true);
        workflow->addControlDependency(t1, t4, true);
        workflow->addControlDependency(t2, t3, true);
        workflow->addControlDependency(t2, t4, true);
        workflow->addControlDependency(t3, t4, true);
    }

    // data members
    std::shared_ptr<wrench::Workflow> workflow;
    std::shared_ptr<wrench::WorkflowTask> t1, t2, t3, t4;
};

TEST_F(AllDependenciesWorkflowTest, AllDependenciesWorkflowStructure) {
    ASSERT_EQ(4, workflow->getNumberOfTasks());

    // testing number of task1's parents
    ASSERT_EQ(0, workflow->getTaskParents(t1).size());
    ASSERT_EQ(1, workflow->getTaskParents(t2).size());
    ASSERT_EQ(2, workflow->getTaskParents(t3).size());
    ASSERT_EQ(3, workflow->getTaskParents(t4).size());

    // testing number of task1's children
    ASSERT_EQ(3, workflow->getTaskChildren(t1).size());
    ASSERT_EQ(2, workflow->getTaskChildren(t2).size());
    ASSERT_EQ(1, workflow->getTaskChildren(t3).size());
    ASSERT_EQ(0, workflow->getTaskChildren(t4).size());

    // testing top-levels
    ASSERT_EQ(0, t1->getTopLevel());
    ASSERT_EQ(1, t2->getTopLevel());
    ASSERT_EQ(2, t3->getTopLevel());
    ASSERT_EQ(3, t4->getTopLevel());

    ASSERT_EQ(4, workflow->getNumLevels());

    // remove tasks
    workflow->removeTask(t4);
    ASSERT_EQ(0, workflow->getTaskChildren(t3).size());
    ASSERT_EQ(1, workflow->getTaskChildren(t2).size());

    ASSERT_EQ(3, workflow->getTasks().size());

    workflow->removeTask(t1);
}

TEST_F(WorkflowTest, LowLevelDagOfTasksTest) {
    wrench::DagOfTasks dag;

    ASSERT_NO_THROW(dag.addVertex((wrench::WorkflowTask *) 1));
    ASSERT_NO_THROW(dag.removeVertex((wrench::WorkflowTask *) 1));
    ASSERT_THROW(dag.removeVertex((wrench::WorkflowTask *) 1), std::invalid_argument);
    ASSERT_NO_THROW(dag.addVertex((wrench::WorkflowTask *) 1));
    ASSERT_NO_THROW(dag.addVertex((wrench::WorkflowTask *) 2));

    ASSERT_NO_THROW(dag.addEdge((wrench::WorkflowTask *) 1, (wrench::WorkflowTask *) 2));
    ASSERT_NO_THROW(dag.removeEdge((wrench::WorkflowTask *) 1, (wrench::WorkflowTask *) 2));
    ASSERT_THROW(dag.removeEdge((wrench::WorkflowTask *) 1, (wrench::WorkflowTask *) 3), std::invalid_argument);
    ASSERT_THROW(dag.removeEdge((wrench::WorkflowTask *) 3, (wrench::WorkflowTask *) 1), std::invalid_argument);
    ASSERT_THROW(dag.addEdge((wrench::WorkflowTask *) 1, (wrench::WorkflowTask *) 3), std::invalid_argument);
    ASSERT_THROW(dag.addEdge((wrench::WorkflowTask *) 3, (wrench::WorkflowTask *) 1), std::invalid_argument);

    ASSERT_NO_THROW(dag.doesPathExist((wrench::WorkflowTask *) 1, (wrench::WorkflowTask *) 2));
    ASSERT_THROW(dag.doesPathExist((wrench::WorkflowTask *) 1, (wrench::WorkflowTask *) 3), std::invalid_argument);
    ASSERT_THROW(dag.doesPathExist((wrench::WorkflowTask *) 3, (wrench::WorkflowTask *) 1), std::invalid_argument);

    ASSERT_NO_THROW(dag.doesEdgeExist((wrench::WorkflowTask *) 1, (wrench::WorkflowTask *) 2));
    ASSERT_THROW(dag.doesEdgeExist((wrench::WorkflowTask *) 1, (wrench::WorkflowTask *) 3), std::invalid_argument);
    ASSERT_THROW(dag.doesEdgeExist((wrench::WorkflowTask *) 3, (wrench::WorkflowTask *) 1), std::invalid_argument);

    ASSERT_NO_THROW(dag.getNumberOfChildren((wrench::WorkflowTask *) 1));
    ASSERT_THROW(dag.getNumberOfChildren((wrench::WorkflowTask *) 3), std::invalid_argument);

    ASSERT_NO_THROW(dag.getChildren((wrench::WorkflowTask *) 1));
    ASSERT_THROW(dag.getChildren((wrench::WorkflowTask *) 3), std::invalid_argument);

    ASSERT_NO_THROW(dag.getNumberOfParents((wrench::WorkflowTask *) 1));
    ASSERT_THROW(dag.getNumberOfParents((wrench::WorkflowTask *) 3), std::invalid_argument);

    ASSERT_NO_THROW(dag.getParents((wrench::WorkflowTask *) 1));
    ASSERT_THROW(dag.getParents((wrench::WorkflowTask *) 3), std::invalid_argument);
}
