#include <wrench/services/compute/serverless/ServerlessComputeService.h>
#include <wrench/managers/function_manager/Function.h>
#include <wrench/logging/TerminalOutput.h>

#include "wrench/services/ServiceMessage.h"
#include "wrench/simulation/Simulation.h"

WRENCH_LOG_CATEGORY(wrench_core_serverless_service, "Log category for Serverless Compute Service");

namespace wrench
{
    ServerlessComputeService::ServerlessComputeService(const std::string& hostname,
                                                       std::vector<std::string> compute_hosts,
                                                       std::string scratch_space_mount_point,
                                                       WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                       WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list) :
        ComputeService(hostname,
                       "ServerlessComputeService",
                       scratch_space_mount_point)
    {
        _compute_hosts = compute_hosts;
    }

    /**
     * @brief Returns true if the service supports standard jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsStandardJobs()
    {
        return false;
    }

    /**
     * @brief Returns true if the service supports compound jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsCompoundJobs()
    {
        return false;
    }

    /**
     * @brief Returns true if the service supports pilot jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsPilotJobs()
    {
        return false;
    }

    /**
     * @brief Method to submit a compound job to the service
     *
     * @param job: The job being submitted
     * @param service_specific_args: the set of service-specific arguments
     */
    void ServerlessComputeService::submitCompoundJob(std::shared_ptr<CompoundJob> job,
                                                     const std::map<std::string, std::string>& service_specific_args)
    {
        throw std::runtime_error("ServerlessComputeService::submitCompoundJob: should not be called");
    }

    /**
     * @brief Method to terminate a compound job at the service
     *
     * @param job: The job being submitted
     */
    void ServerlessComputeService::terminateCompoundJob(std::shared_ptr<CompoundJob> job)
    {
        throw std::runtime_error("ServerlessComputeService::terminateCompoundJob: should not be called");
    }

    /**
     * @brief Construct a dict for resource information
     * @param key: the desired key
     * @return a dictionary
     */
    std::map<std::string, double> ServerlessComputeService::constructResourceInformation(const std::string& key)
    {
        throw std::runtime_error("ServerlessComputeService::constructResourceInformation: not implemented");
    }

    void ServerlessComputeService::registerFunction(Function function, double time_limit_in_seconds,
                                                    sg_size_t disk_space_limit_in_bytes, sg_size_t RAM_limit_in_bytes,
                                                    sg_size_t ingress_in_bytes, sg_size_t egress_in_bytes)
    {
        WRENCH_INFO(("Serverless Provider Registered function " + function.getName()).c_str());
        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        //  send a "run a standard job" message to the daemon's commport
        this->commport->putMessage(
            new ServerlessComputeServiceFunctionRegisterRequestMessage(
                answer_commport, function, XXXX,
                this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        auto msg = answer_commport->getMessage<ServerlessComputeServiceFunctionRegisterAnswerMessage>(
            this->network_timeout,
            "ServerlessComputeService::registerFunction(): Received an");

        // TODO: Deal with failures later
        // if (not msg->success) {
        //     throw ExecutionException(msg->failure_cause);
        // }
    }

    int ServerlessComputeService::main()
    {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        WRENCH_INFO("Serverless provider starting");

        while (processNextMessage()) {
            // Do stuff if needed
        }
        return 0;
    }

    bool ServerlessComputeService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        } catch (ExecutionException &e) {
            WRENCH_INFO(
                    "Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());
        //        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (auto ss_mesg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message))
        {
            // TODO: Die...
        }
        if (auto scsfrrm_msg = std::dynamic_pointer_cast<ServerlessComputeServiceFunctionRegistrationRequestMessage>(message))
        {
            processFunctionRegistrationRequest(message->answer_commport, message->function, message->time_limit_in_seconds,....);
        } else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }

    }

    void ServerlessComputeService::processFunctionRegistrationRequest(...)
    {
        // TODO: Do the registration
        // TODO: Reply to the request
    }
};