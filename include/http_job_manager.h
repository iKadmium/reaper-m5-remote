#pragma once

#include "hal_interfaces.h"
#include "reaper_types.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>

#ifdef ARDUINO
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#else
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#endif

namespace http
{
    // Forward declarations
    class HttpJob;
    class HttpJobResult;

    // Result type enumeration for type identification without RTTI
    enum class ResultType
    {
        CHANGE_TAB,
        CHANGE_PLAYSTATE,
        GET_STATUS,
        GET_SCRIPT_ACTION_ID,
        GET_TRANSPORT
    };

    // Base class for HTTP job results
    class HttpJobResult
    {
    public:
        uint32_t job_id;
        bool success;
        uint32_t timestamp;
        ResultType result_type;

        HttpJobResult(uint32_t id, ResultType type) : job_id(id), success(false), timestamp(0), result_type(type) {}
        virtual ~HttpJobResult() = default;
    };

    // Base class for HTTP jobs
    class HttpJob
    {
    public:
        uint32_t job_id;
        uint32_t timestamp;

        HttpJob(uint32_t id) : job_id(id), timestamp(0) {}
        virtual ~HttpJob() = default;

        // Pure virtual method to execute the job and return the result
        virtual std::unique_ptr<HttpJobResult> execute(hal::INetworkManager *network_mgr, const std::string &base_url) = 0;
        virtual const char *getJobTypeName() const = 0;
    };

    // Change Tab Job and Result
    enum class TabDirection
    {
        NEXT,
        PREVIOUS
    };

    class ChangeTabResult : public HttpJobResult
    {
    public:
        reaper::ReaperState reaper_state;
        reaper::TransportState transport_state;

        ChangeTabResult(uint32_t id) : HttpJobResult(id, ResultType::CHANGE_TAB)
        {
            reaper_state = reaper::ReaperState{};
            transport_state = reaper::TransportState{};
        }
    };

    class ChangeTabJob : public HttpJob
    {
    private:
        TabDirection direction;
        std::string script_action_id;

    public:
        ChangeTabJob(uint32_t id, TabDirection dir, const std::string &script_id)
            : HttpJob(id), direction(dir), script_action_id(script_id) {}

        std::unique_ptr<HttpJobResult> execute(hal::INetworkManager *network_mgr, const std::string &base_url) override;
        const char *getJobTypeName() const override { return "ChangeTab"; }
    };

    // Change Playstate Job and Result
    enum class PlayAction
    {
        PLAY,
        STOP
    };

    class ChangePlaystateResult : public HttpJobResult
    {
    public:
        reaper::TransportState transport_state;

        ChangePlaystateResult(uint32_t id) : HttpJobResult(id, ResultType::CHANGE_PLAYSTATE)
        {
            transport_state = reaper::TransportState{};
        }
    };

    class ChangePlaystateJob : public HttpJob
    {
    private:
        PlayAction action;

    public:
        ChangePlaystateJob(uint32_t id, PlayAction act) : HttpJob(id), action(act) {}

        std::unique_ptr<HttpJobResult> execute(hal::INetworkManager *network_mgr, const std::string &base_url) override;
        const char *getJobTypeName() const override { return "ChangePlaystate"; }
    };

    // Get Status Job and Result
    class GetStatusResult : public HttpJobResult
    {
    public:
        reaper::ReaperState reaper_state;
        reaper::TransportState transport_state;

        GetStatusResult(uint32_t id) : HttpJobResult(id, ResultType::GET_STATUS)
        {
            reaper_state = reaper::ReaperState{};
            transport_state = reaper::TransportState{};
        }
    };

    class GetScriptActionIdResult : public HttpJobResult
    {
    public:
        std::string script_action_id;

        GetScriptActionIdResult(uint32_t id) : HttpJobResult(id, ResultType::GET_SCRIPT_ACTION_ID) {}
    };

    class GetTransportResult : public HttpJobResult
    {
    public:
        reaper::TransportState transport_state;

        GetTransportResult(uint32_t id) : HttpJobResult(id, ResultType::GET_TRANSPORT)
        {
            transport_state = reaper::TransportState{};
        }
    };

    class GetStatusJob : public HttpJob
    {
    private:
        std::string script_action_id;

    public:
        GetStatusJob(uint32_t id, const std::string &script_id)
            : HttpJob(id), script_action_id(script_id) {}

        std::unique_ptr<HttpJobResult> execute(hal::INetworkManager *network_mgr, const std::string &base_url) override;
        const char *getJobTypeName() const override { return "GetStatus"; }
    };

    class GetScriptActionIdJob : public HttpJob
    {
    public:
        GetScriptActionIdJob(uint32_t id) : HttpJob(id) {}

        std::unique_ptr<HttpJobResult> execute(hal::INetworkManager *network_mgr, const std::string &base_url) override;
        const char *getJobTypeName() const override { return "GetScriptActionId"; }
    };

    class GetTransportJob : public HttpJob
    {
    public:
        GetTransportJob(uint32_t id) : HttpJob(id) {}

        std::unique_ptr<HttpJobResult> execute(hal::INetworkManager *network_mgr, const std::string &base_url) override;
        const char *getJobTypeName() const override { return "GetTransport"; }
    };

    class HttpJobManager
    {
    private:
        hal::ISystemHAL *system_hal;
        std::string base_url;
        std::string script_action_id; // ReaperSetlist script action ID

        std::atomic<uint32_t> next_job_id;
        bool worker_running;

        // Results queue owned by main thread
#ifdef ARDUINO
        QueueHandle_t main_result_queue; // Main thread receives from this
#else
        std::queue<std::unique_ptr<HttpJobResult>> results;
        std::mutex results_mutex; // Minimal sync for cross-thread communication
#endif

#ifdef ARDUINO
        // FreeRTOS primitives
        QueueHandle_t job_queue;
        TaskHandle_t worker_task_handle;

        static void workerTaskWrapper(void *parameter);
        void workerTask();
#else
        // Standard library primitives
        std::queue<std::unique_ptr<HttpJob>> job_queue;
        std::thread worker_thread;
        std::condition_variable job_available;
        std::mutex job_queue_mutex;
        std::atomic<bool> should_stop;

        void workerThreadFunction();
#endif

        uint32_t generateJobId();

        // Worker thread sends results to main thread
        void sendResult(std::unique_ptr<HttpJobResult> result);

    public:
        HttpJobManager(hal::ISystemHAL *system, const std::string &reaper_base_url);
        ~HttpJobManager();

        // Lifecycle
        bool initialize();
        void shutdown();

        // Job submission - returns job ID, no callbacks needed
        uint32_t submitChangeTabJob(TabDirection direction);
        uint32_t submitChangePlaystateJob(PlayAction action);
        uint32_t submitGetStatusJob();
        uint32_t submitGetScriptActionIdJob();
        uint32_t submitGetTransportJob();

        // Result processing (call from main thread) - returns ownership of results
        std::vector<std::unique_ptr<HttpJobResult>> processResults();

        // Script action ID management
        void setScriptActionId(const std::string &id) { script_action_id = id; }
        const std::string &getScriptActionId() const { return script_action_id; }

        // Status
        bool isWorkerRunning() const { return worker_running; }
    };

} // namespace http
