/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_COMPUTEACTION_H
#define WRENCH_COMPUTEACTION_H

#include <string>

#include "wrench/action/Action.h"

namespace wrench {


    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class ParallelModel;

    /**
     * @brief A class that implements a compute action
     */
    class ComputeAction : public Action {

    public:
        double getFlops() const;
        unsigned long getMinNumCores() const override;
        unsigned long getMaxNumCores() const override;
        sg_size_t getMinRAMFootprint() const override;
        std::shared_ptr<ParallelModel> getParallelModel() const;

    protected:
        friend class CompoundJob;

        ComputeAction(const std::string &name,
                      double flops,
                      sg_size_t ram,
                      unsigned long min_num_cores,
                      unsigned long max_num_cores,
                      std::shared_ptr<ParallelModel> parallel_model);

        void execute(const std::shared_ptr<ActionExecutor> &action_executor) override;
        void terminate(const std::shared_ptr<ActionExecutor> &action_executor) override;

    private:
        double flops;
        unsigned long min_num_cores;
        unsigned long max_num_cores;
        sg_size_t ram;
        std::shared_ptr<ParallelModel> parallel_model;

        static void simulateComputationAsSleep(const std::shared_ptr<ActionExecutor> &action_executor, unsigned long num_threads, double sequential_work, double parallel_per_thread_work);
        static void simulateComputationAsComputation(const std::shared_ptr<ActionExecutor> &action_executor, unsigned long num_threads, double sequential_work, double parallel_per_thread_work);
    };


    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_COMPUTEACTION_H
