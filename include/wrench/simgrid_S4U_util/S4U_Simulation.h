/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_S4U_SIMULATION_H
#define WRENCH_S4U_SIMULATION_H

#include <climits>
#include <cfloat>
#include <limits>
#include <simgrid/s4u.hpp>
#include <simgrid/kernel/routing/ClusterZone.hpp>

namespace wrench {


    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Wrappers around S4U's basic simulation methods
     */
    class S4U_Simulation {
    public:
        /** @brief The ram capacity of a physical host whenever not specified in the platform description file */
        static constexpr sg_size_t DEFAULT_RAM = LLONG_MAX;
        static constexpr double RAM_READ_BANDWIDTH = DBL_MAX;
        static constexpr double RAM_WRITE_BANDWIDTH = DBL_MAX;

    public:
        static void enableSMPI();
        void initialize(int *argc, char **argv);
        void setupPlatformFromFile(const std::string &filepath);
        void setupPlatformFromLambda(const std::function<void()> &creation_function);
        void runSimulation();
        static double getClock();
        static std::string getHostName();
        static bool hostExists(const std::string &hostname);
        static bool linkExists(const std::string &link_name);
        static std::vector<std::string> getRoute(const std::string &src_host, const std::string &dst_host);
        static unsigned int getHostNumCores(const std::string &hostname);
        static unsigned int getNumCores();
        static double getHostFlopRate(const std::string &hostname);
        static bool isHostOn(const std::string &hostname);
        static void turnOffHost(const std::string &hostname);
        static void turnOnHost(const std::string &hostname);
        static bool isLinkOn(const std::string &link_name);
        static void turnOffLink(const std::string &link_name);
        static void turnOnLink(const std::string &link_name);
        static double getFlopRate();
        static sg_size_t getHostMemoryCapacity(const std::string &hostname);
        static sg_size_t getMemoryCapacity();
        static void compute(double);
        static void compute_multi_threaded(unsigned long num_threads,
                                           double thread_creation_overhead,
                                           double sequential_work,
                                           double parallel_per_thread_work);
        static void sleep(double);
        static void computeZeroFlop();
        static void writeToDisk(sg_size_t num_bytes, const std::string &hostname, std::string mount_point, simgrid::s4u::Disk *disk);
        static void readFromDisk(sg_size_t num_bytes, const std::string &hostname, std::string mount_point, simgrid::s4u::Disk *disk);
        static void readFromDiskAndWriteToDiskConcurrently(sg_size_t num_bytes_to_read, sg_size_t num_bytes_to_write,
                                                           const std::string &hostname,
                                                           const std::string &read_mount_point,
                                                           const std::string &write_mount_point,
                                                           simgrid::s4u::Disk *src_disk,
                                                           simgrid::s4u::Disk *dst_disk);

        static sg_size_t getDiskCapacity(const std::string &hostname, std::string mount_point);
        static std::vector<std::string> getDisks(const std::string &hostname);
        static simgrid::s4u::Disk *hostHasMountPoint(const std::string &hostname, const std::string &mount_point);

        void checkLinkBandwidths() const;

        static void yield();
        static std::string getHostProperty(const std::string &hostname, const std::string &property_name);
        static void setHostProperty(const std::string &hostname, const std::string &property_name, const std::string &property_value);
        static std::string getClusterProperty(const std::string &cluster_id, const std::string &property_name);

        //start energy related calls
        static double getEnergyConsumedByHost(const std::string &hostname);
        //		static double getTotalEnergyConsumed(const std::vector<std::string> &hostnames);
        static void setPstate(const std::string &hostname, unsigned long pstate);
        static int getNumberOfPstates(const std::string &hostname);
        static unsigned long getCurrentPstate(const std::string &hostname);
        static double getMinPowerConsumption(const std::string &hostname);
        static double getMaxPowerConsumption(const std::string &hostname);
        static std::vector<int> getListOfPstates(const std::string &hostname);
        //end energy related calls

        bool isInitialized() const;
        bool isPlatformSetup() const;
        static std::vector<std::string> getAllHostnames();
        static std::vector<std::string> getAllLinknames();
        static double getLinkBandwidth(const std::string &name);
        static void setLinkBandwidth(const std::string &name, double bandwidth);
        static double getLinkUsage(const std::string &name);

        static std::map<std::string, std::vector<std::string>> getAllHostnamesByCluster();
        static std::map<std::string, std::vector<std::string>> getAllHostnamesByZone();
        static std::map<std::string, std::vector<std::string>> getAllClusterIDsByZone();
        static std::map<std::string, std::vector<std::string>> getAllSubZoneIDsByZone();

        static void createNewDisk(const std::string &hostname, const std::string &disk_id, double read_bandwidth_in_bytes_per_sec, double write_bandwidth_in_bytes_per_sec, sg_size_t capacity_in_bytes, const std::string &mount_point);

        void shutdown() const;

        static simgrid::s4u::Host *get_host_or_vm_by_name_or_null(const std::string &name);
        static simgrid::s4u::Host *get_host_or_vm_by_name(const std::string &name);

        static sg_size_t getHostMemoryCapacity(simgrid::s4u::Host *host);

        // static simgrid::s4u::MutexPtr global_lock;

    private:
        static void traverseAllNetZonesRecursive(simgrid::s4u::NetZone *nz, std::map<std::string,
                                                 std::vector<std::string>> &result,
                                                 bool get_subzones, bool get_clusters,
                                                 bool get_hosts_from_zones, bool get_hosts_from_clusters);

        simgrid::s4u::Engine *engine = nullptr;
        bool initialized = false;
        bool platform_setup = false;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench

#endif//WRENCH_S4U_SIMULATION_H
