#include "http_job_manager.h"
#include "network_manager.h"
#include "log.h"
#include <atomic>
#include <stdexcept>

namespace http
{

#ifdef ARDUINO
    // Queue sizes for FreeRTOS
    static const int JOB_QUEUE_SIZE = 10;
    static const int RESULT_QUEUE_SIZE = 10;
    static const int WORKER_STACK_SIZE = 8192;
    static const int WORKER_PRIORITY = 1;
#endif

    // HttpJobManager implementation
    HttpJobManager::HttpJobManager(hal::ISystemHAL *system, NetworkManager *network, const std::string &reaper_base_url)
        : system_hal(system), network_manager(network), base_url(reaper_base_url),
          wifi_connected(false), last_wifi_attempt(0), script_action_id_attempts(0),
          next_job_id(1), worker_running(false)
#ifdef ARDUINO
          ,
          job_queue(nullptr), main_result_queue(nullptr), worker_task_handle(nullptr)
#else
          ,
          should_stop(false)
#endif
    {
        LOG_INFO("HttpJobManager", "Initializing HTTP job manager");

#ifdef ARDUINO
        // Create FreeRTOS queue for jobs
        job_queue = xQueueCreate(JOB_QUEUE_SIZE, sizeof(HttpJob *));
        if (!job_queue)
        {
            LOG_ERROR("HttpJobManager", "Failed to create job queue");
            throw std::runtime_error("Failed to create job queue");
        }

        // Create FreeRTOS queue for results (main thread receives from this)
        main_result_queue = xQueueCreate(RESULT_QUEUE_SIZE, sizeof(HttpJobResult *));
        if (!main_result_queue)
        {
            LOG_ERROR("HttpJobManager", "Failed to create result queue");
            vQueueDelete(job_queue);
            job_queue = nullptr;
            throw std::runtime_error("Failed to create result queue");
        }

        // Create worker task
        BaseType_t result = xTaskCreate(
            workerTaskWrapper,
            "http_worker",
            WORKER_STACK_SIZE,
            this,
            WORKER_PRIORITY,
            &worker_task_handle);

        if (result != pdPASS)
        {
            LOG_ERROR("HttpJobManager", "Failed to create worker task");
            vQueueDelete(job_queue);
            vQueueDelete(main_result_queue);
            job_queue = nullptr;
            main_result_queue = nullptr;
            throw std::runtime_error("Failed to create worker task");
        }
#else
        // Create worker thread
        try
        {
            worker_thread = std::thread(&HttpJobManager::workerThreadFunction, this);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("HttpJobManager", "Failed to create worker thread: {}", e.what());
            throw;
        }
#endif

        worker_running = true;

        // Submit WiFi connection job as the first job
        uint32_t wifi_job_id = submitWiFiConnectJob();
        LOG_INFO("HttpJobManager", "Submitted initial WiFi connection job {}", wifi_job_id);

        LOG_INFO("HttpJobManager", "HTTP job manager initialized successfully");
    }

    HttpJobManager::~HttpJobManager()
    {
        shutdown();
    }

    void HttpJobManager::shutdown()
    {
        if (!worker_running)
            return;

        LOG_INFO("HttpJobManager", "Shutting down HTTP job manager");
        worker_running = false;

#ifdef ARDUINO
        if (worker_task_handle)
        {
            vTaskDelete(worker_task_handle);
            worker_task_handle = nullptr;
        }

        if (job_queue)
        {
            vQueueDelete(job_queue);
            job_queue = nullptr;
        }

        if (main_result_queue)
        {
            vQueueDelete(main_result_queue);
            main_result_queue = nullptr;
        }
#else
        should_stop = true;
        job_available.notify_all();

        if (worker_thread.joinable())
        {
            worker_thread.join();
        }
#endif

        LOG_INFO("HttpJobManager", "HTTP job manager shutdown complete");
    }

    uint32_t HttpJobManager::generateJobId()
    {
        return next_job_id.fetch_add(1);
    }

    void HttpJobManager::sendResult(std::unique_ptr<HttpJobResult> result)
    {
        // Worker thread sends result to main thread's queue
#ifdef ARDUINO
        HttpJobResult *result_ptr = result.release();
        if (xQueueSend(main_result_queue, &result_ptr, 0) != pdTRUE)
        {
            LOG_ERROR("HttpJobManager", "Failed to send result for job {} - main queue full", result_ptr->job_id);
            delete result_ptr; // Clean up if queue is full
        }
#else
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push(std::move(result));
#endif
    }

    uint32_t HttpJobManager::submitWiFiConnectJob()
    {
        if (!worker_running)
        {
            LOG_ERROR("HttpJobManager", "Cannot submit job - worker not running");
            return 0;
        }

        uint32_t job_id = generateJobId();
        auto job = std::unique_ptr<WiFiConnectJob>(new WiFiConnectJob(job_id, network_manager));
        job->timestamp = system_hal->getMillis();

#ifdef ARDUINO
        HttpJob *job_ptr = job.release();
        if (xQueueSend(job_queue, &job_ptr, 0) != pdTRUE)
        {
            LOG_ERROR("HttpJobManager", "Failed to submit WiFi connect job - queue full");
            delete job_ptr;
            return 0;
        }
#else
        {
            std::lock_guard<std::mutex> lock(job_queue_mutex);
            job_queue.push(std::move(job));
        }
        job_available.notify_one();
#endif

        LOG_DEBUG("HttpJobManager", "Submitted WiFi connect job {}", job_id);
        return job_id;
    }

    uint32_t HttpJobManager::submitChangeTabJob(TabDirection direction)
    {
        if (!worker_running)
        {
            LOG_ERROR("HttpJobManager", "Cannot submit job - worker not running");
            return 0;
        }

        uint32_t job_id = generateJobId();
        auto job = std::unique_ptr<ChangeTabJob>(new ChangeTabJob(job_id, direction, script_action_id));
        job->timestamp = system_hal->getMillis();

#ifdef ARDUINO
        HttpJob *job_ptr = job.release();
        if (xQueueSend(job_queue, &job_ptr, 0) != pdTRUE)
        {
            LOG_ERROR("HttpJobManager", "Failed to submit change tab job - queue full");
            delete job_ptr;
            return 0;
        }
#else
        {
            std::lock_guard<std::mutex> lock(job_queue_mutex);
            job_queue.push(std::move(job));
        }
        job_available.notify_one();
#endif

        LOG_DEBUG("HttpJobManager", "Submitted change tab job {} (direction: {})",
                  job_id, direction == TabDirection::NEXT ? "NEXT" : "PREVIOUS");
        return job_id;
    }

    uint32_t HttpJobManager::submitChangePlaystateJob(PlayAction action)
    {
        if (!worker_running)
        {
            LOG_ERROR("HttpJobManager", "Cannot submit job - worker not running");
            return 0;
        }

        uint32_t job_id = generateJobId();
        auto job = std::unique_ptr<ChangePlaystateJob>(new ChangePlaystateJob(job_id, action));
        job->timestamp = system_hal->getMillis();

#ifdef ARDUINO
        HttpJob *job_ptr = job.release();
        if (xQueueSend(job_queue, &job_ptr, 0) != pdTRUE)
        {
            LOG_ERROR("HttpJobManager", "Failed to submit change playstate job - queue full");
            delete job_ptr;
            return 0;
        }
#else
        {
            std::lock_guard<std::mutex> lock(job_queue_mutex);
            job_queue.push(std::move(job));
        }
        job_available.notify_one();
#endif

        LOG_DEBUG("HttpJobManager", "Submitted change playstate job {} (action: {})",
                  job_id, static_cast<int>(action));
        return job_id;
    }

    uint32_t HttpJobManager::submitGetStatusJob()
    {
        if (!worker_running)
        {
            LOG_ERROR("HttpJobManager", "Cannot submit job - worker not running");
            return 0;
        }

        uint32_t job_id = generateJobId();
        auto job = std::unique_ptr<GetStatusJob>(new GetStatusJob(job_id, script_action_id));
        job->timestamp = system_hal->getMillis();

#ifdef ARDUINO
        HttpJob *job_ptr = job.release();
        if (xQueueSend(job_queue, &job_ptr, 0) != pdTRUE)
        {
            LOG_ERROR("HttpJobManager", "Failed to submit get status job - queue full");
            delete job_ptr;
            return 0;
        }
#else
        {
            std::lock_guard<std::mutex> lock(job_queue_mutex);
            job_queue.push(std::move(job));
        }
        job_available.notify_one();
#endif

        LOG_DEBUG("HttpJobManager", "Submitted get status job {}", job_id);
        return job_id;
    }

    uint32_t HttpJobManager::submitGetScriptActionIdJob()
    {
        if (!worker_running)
        {
            LOG_ERROR("HttpJobManager", "Cannot submit job - worker not running");
            return 0;
        }

        uint32_t job_id = generateJobId();
        auto job = std::unique_ptr<GetScriptActionIdJob>(new GetScriptActionIdJob(job_id));
        job->timestamp = system_hal->getMillis();

#ifdef ARDUINO
        HttpJob *job_ptr = job.release();
        if (xQueueSend(job_queue, &job_ptr, 0) != pdTRUE)
        {
            LOG_ERROR("HttpJobManager", "Failed to submit get script action ID job - queue full");
            delete job_ptr;
            return 0;
        }
#else
        {
            std::lock_guard<std::mutex> lock(job_queue_mutex);
            job_queue.push(std::move(job));
        }
        job_available.notify_one();
#endif

        LOG_DEBUG("HttpJobManager", "Submitted get script action ID job {}", job_id);
        return job_id;
    }

    uint32_t HttpJobManager::submitGetTransportJob()
    {
        if (!worker_running)
        {
            LOG_ERROR("HttpJobManager", "Cannot submit job - worker not running");
            return 0;
        }

        uint32_t job_id = generateJobId();
        auto job = std::unique_ptr<GetTransportJob>(new GetTransportJob(job_id));
        job->timestamp = system_hal->getMillis();

#ifdef ARDUINO
        HttpJob *job_ptr = job.release();
        if (xQueueSend(job_queue, &job_ptr, 0) != pdTRUE)
        {
            LOG_ERROR("HttpJobManager", "Failed to submit get transport job - queue full");
            delete job_ptr;
            return 0;
        }
#else
        {
            std::lock_guard<std::mutex> lock(job_queue_mutex);
            job_queue.push(std::move(job));
        }
        job_available.notify_one();
#endif

        LOG_DEBUG("HttpJobManager", "Submitted get transport job {}", job_id);
        return job_id;
    }

    void HttpJobManager::checkAndRetryConnections(uint32_t current_time)
    {
        if (!worker_running)
            return;

        // Check if we need to retry WiFi connection
        if (!wifi_connected.load())
        {
            uint32_t last_attempt = last_wifi_attempt.load();
            if (current_time - last_attempt >= WIFI_RETRY_INTERVAL_MS)
            {
                LOG_DEBUG("HttpJobManager", "WiFi not connected, retrying...");
                submitWiFiConnectJob();
                last_wifi_attempt.store(current_time);
            }
        }
        // If WiFi is connected but we don't have script action ID, retry that
        else if (wifi_connected.load() && script_action_id.empty())
        {
            uint32_t attempts = script_action_id_attempts.load();
            if (attempts < MAX_SCRIPT_ID_ATTEMPTS)
            {
                LOG_DEBUG("HttpJobManager", "Retrying script action ID request (attempt {})", attempts + 1);
                submitGetScriptActionIdJob();
                script_action_id_attempts.store(attempts + 1);
            }
            else
            {
                LOG_ERROR("HttpJobManager", "Max script action ID attempts reached, giving up");
            }
        }
    }

    std::vector<std::unique_ptr<HttpJobResult>> HttpJobManager::processResults()
    {
        std::vector<std::unique_ptr<HttpJobResult>> results_vec;

        if (!worker_running)
            return results_vec;

#ifdef ARDUINO
        HttpJobResult *result_ptr;
        while (xQueueReceive(main_result_queue, &result_ptr, 0) == pdTRUE)
        {
            LOG_DEBUG("HttpJobManager", "Processing result for job {}", result_ptr->job_id);
            results_vec.emplace_back(result_ptr);
        }
#else
        std::lock_guard<std::mutex> lock(results_mutex);
        while (!results.empty())
        {
            auto result = std::move(results.front());
            results.pop();
            LOG_DEBUG("HttpJobManager", "Processing result for job {}", result->job_id);
            results_vec.push_back(std::move(result));
        }
#endif

        return results_vec;
    }

#ifdef ARDUINO
    void HttpJobManager::workerTaskWrapper(void *parameter)
    {
        HttpJobManager *manager = static_cast<HttpJobManager *>(parameter);
        manager->workerTask();
        vTaskDelete(nullptr);
    }

    void HttpJobManager::workerTask()
    {
        LOG_INFO("HttpJobManager", "Worker task started");
        HttpJob *job_ptr;

        while (worker_running)
        {
            if (xQueueReceive(job_queue, &job_ptr, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                LOG_DEBUG("HttpJobManager", "Processing job {} of type {}", job_ptr->job_id, job_ptr->getJobTypeName());

                // Execute the job
                auto result = job_ptr->execute(&system_hal->getNetworkManager(), base_url);
                result->timestamp = system_hal->getMillis();

                // Update connection state if this was a WiFi job
                if (result->result_type == http::ResultType::WIFI_CONNECT)
                {
                    auto wifi_result = static_cast<http::WiFiConnectResult *>(result.get());
                    wifi_connected.store(wifi_result->connected);
                    if (wifi_result->connected)
                    {
                        last_wifi_attempt.store(0); // Reset retry timer
                    }
                }

                // Send result back
                sendResult(std::move(result));

                delete job_ptr;
            }
        }

        LOG_INFO("HttpJobManager", "Worker task ended");
    }
#else
    void HttpJobManager::workerThreadFunction()
    {
        LOG_INFO("HttpJobManager", "Worker thread started");

        while (!should_stop)
        {
            std::unique_ptr<HttpJob> job;

            // Wait for a job
            {
                std::unique_lock<std::mutex> lock(job_queue_mutex);
                job_available.wait(lock, [this]
                                   { return !job_queue.empty() || should_stop; });

                if (should_stop)
                    break;

                if (!job_queue.empty())
                {
                    job = std::move(job_queue.front());
                    job_queue.pop();
                }
            }

            if (job)
            {
                LOG_DEBUG("HttpJobManager", "Processing job {} of type {}", job->job_id, job->getJobTypeName());

                // Execute the job
                auto result = job->execute(&system_hal->getNetworkManager(), base_url);
                result->timestamp = system_hal->getMillis();

                // Update connection state if this was a WiFi job
                if (result->result_type == http::ResultType::WIFI_CONNECT)
                {
                    auto wifi_result = static_cast<http::WiFiConnectResult *>(result.get());
                    wifi_connected.store(wifi_result->connected);
                    if (wifi_result->connected)
                    {
                        last_wifi_attempt.store(0); // Reset retry timer
                    }
                }

                // Send result back
                sendResult(std::move(result));
            }
        }

        LOG_INFO("HttpJobManager", "Worker thread ended");
    }
#endif

} // namespace http
