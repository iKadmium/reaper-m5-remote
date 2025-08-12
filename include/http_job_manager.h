#pragma once

#include "hal_interfaces.h"
#include "http_jobs.h"
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
    class HttpJobManager
    {
    private:
        hal::ISystemHAL *system_hal;
        NetworkManager *network_manager;
        std::string base_url;
        std::string script_action_id; // ReaperSetlist script action ID

        // Connection state tracking
        std::atomic<bool> wifi_connected;
        std::atomic<uint32_t> last_wifi_attempt;
        std::atomic<uint32_t> script_action_id_attempts;
        static const uint32_t WIFI_RETRY_INTERVAL_MS = 10000;     // 10 seconds
        static const uint32_t SCRIPT_ID_RETRY_INTERVAL_MS = 5000; // 5 seconds
        static const uint32_t MAX_SCRIPT_ID_ATTEMPTS = 5;

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

        // Internal shutdown logic (called from destructor)
        void shutdown();

    public:
        HttpJobManager(hal::ISystemHAL *system, NetworkManager *network, const std::string &reaper_base_url);
        ~HttpJobManager();

        // Non-copyable, non-movable (RAII resource management)
        HttpJobManager(const HttpJobManager &) = delete;
        HttpJobManager &operator=(const HttpJobManager &) = delete;
        HttpJobManager(HttpJobManager &&) = delete;
        HttpJobManager &operator=(HttpJobManager &&) = delete;

        // Job submission - returns job ID, no callbacks needed
        uint32_t submitWiFiConnectJob();
        uint32_t submitChangeTabJob(TabDirection direction);
        uint32_t submitChangePlaystateJob(PlayAction action);
        uint32_t submitGetStatusJob();
        uint32_t submitGetScriptActionIdJob();
        uint32_t submitGetTransportJob();

        // Result processing (call from main thread) - returns ownership of results
        std::vector<std::unique_ptr<HttpJobResult>> processResults();

        // Connection management
        bool isWiFiConnected() const { return wifi_connected.load(); }
        void checkAndRetryConnections(uint32_t current_time);

        // Script action ID management
        void setScriptActionId(const std::string &id)
        {
            script_action_id = id;
            script_action_id_attempts.store(0); // Reset attempts when successful
        }
        const std::string &getScriptActionId() const { return script_action_id; }

        // Status
        bool isWorkerRunning() const { return worker_running; }
    };

} // namespace http
