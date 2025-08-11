#include "http_job_manager.h"
#include "log.h"
#include <cstring>
#include <sstream>
#include <atomic>
#include <ArduinoJson.h>

namespace http
{

#ifdef ARDUINO
    // Queue sizes for FreeRTOS
    static const int JOB_QUEUE_SIZE = 10;
    static const int RESULT_QUEUE_SIZE = 10;
    static const int WORKER_STACK_SIZE = 8192;
    static const int WORKER_PRIORITY = 1;
#endif

    // Reaper command constants
    namespace commands
    {
        // Transport commands
        static const char *TRANSPORT = "TRANSPORT";
        static const char *PLAY = "1007";
        static const char *STOP = "1016";

        // Tab navigation commands
        static const char *NEXT_TAB = "40861";
        static const char *PREVIOUS_TAB = "40862";

        // ReaperSetlist commands
        static const char *GET_SCRIPT_ACTION_ID = "GET/EXTSTATE/ReaperSetlist/ScriptActionId";
        static const char *SET_OPERATION_GET_OPEN_TABS = "SET/EXTSTATE/ReaperSetlist/Operation/getOpenTabs";
        static const char *GET_TABS = "GET/EXTSTATE/ReaperSetlist/tabs";
        static const char *GET_ACTIVE_INDEX = "GET/EXTSTATE/ReaperSetlist/activeIndex";

        // Response prefixes
        static const char *EXTSTATE_PREFIX = "EXTSTATE";
        static const char *REAPER_SETLIST = "ReaperSetlist";
        static const char *SCRIPT_ACTION_ID_KEY = "ScriptActionId";
    }

    // Helper function to parse tab-separated response
    static std::vector<std::string> parseTabSeparatedResponse(const std::string &response)
    {
        std::vector<std::string> items;
        std::stringstream ss(response);
        std::string item;

        while (std::getline(ss, item, '\t'))
        {
            // Remove newline at end if present
            if (!item.empty() && item.back() == '\n')
            {
                item.pop_back();
            }
            if (!item.empty())
            {
                items.push_back(item);
            }
        }

        return items;
    }

    // Helper function to build URL for single command
    static std::string buildCommandUrl(const std::string &base_url, const std::string &command)
    {
        return base_url + "/" + command;
    }

    // Helper function to build URL for multiple commands (separated by semicolons)
    static std::string buildCommandUrl(const std::string &base_url, const std::vector<std::string> &commands)
    {
        if (commands.empty())
        {
            return base_url;
        }

        std::string url = base_url + "/";
        for (size_t i = 0; i < commands.size(); ++i)
        {
            if (i > 0)
            {
                url += ";";
            }
            url += commands[i];
        }
        return url;
    }

    // Helper function to parse newline-separated batch response
    static std::vector<std::string> parseBatchResponse(const std::string &response)
    {
        std::vector<std::string> lines;
        std::stringstream ss(response);
        std::string line;

        while (std::getline(ss, line, '\n'))
        {
            // Remove carriage return if present
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            if (!line.empty())
            {
                lines.push_back(line);
            }
        }

        return lines;
    }

    // Helper function to parse transport state from tab-separated response
    static bool parseTransportState(const std::string &response, reaper::TransportState &transport_state)
    {
        auto items = parseTabSeparatedResponse(response);
        if (items.size() >= 4)
        {
            transport_state.play_state = std::stoi(items[1]);
            transport_state.position_seconds = std::stod(items[2]);
            transport_state.repeat_enabled = (items[3] == "1");
            transport_state.position_bars_beats = items[4];
            transport_state.success = true;
            return true;
        }
        return false;
    }

    // Helper function to parse individual tabs from tab data string
    static std::vector<reaper::TabInfo> parseTabData(const std::string &tab_data)
    {
        std::vector<reaper::TabInfo> tabs;

        // Tab data is JSON format: [{"length":297,"name":"Believer.RPP","index":0,"dirty":false},...]
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, tab_data);

        if (error)
        {
            LOG_ERROR("parseTabData", "Failed to parse JSON: {}", error.c_str());
            return tabs;
        }

        if (!doc.is<JsonArray>())
        {
            LOG_ERROR("parseTabData", "Tab data is not a JSON array");
            return tabs;
        }

        JsonArray tabArray = doc.as<JsonArray>();
        for (JsonObject tabObj : tabArray)
        {
            if (tabObj["length"].is<float>() && tabObj["name"].is<const char *>() && tabObj["index"].is<int>())
            {
                reaper::TabInfo tab;
                try
                {
                    tab.length = tabObj["length"].as<float>();
                    tab.name = tabObj["name"].as<std::string>();
                    tab.index = tabObj["index"].as<int>();
                    tabs.push_back(tab);
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR("parseTabData", "Failed to parse tab object: {}", e.what());
                }
            }
            else
            {
                LOG_WARNING("parseTabData", "Tab object missing required fields");
            }
        }

        LOG_DEBUG("parseTabData", "Successfully parsed {} tabs from JSON", tabs.size());
        return tabs;
    }

    // ChangeTabJob implementation
    std::unique_ptr<HttpJobResult> ChangeTabJob::execute(hal::INetworkManager *network_mgr, const std::string &base_url)
    {
        auto result = std::unique_ptr<ChangeTabResult>(new ChangeTabResult(job_id));

        LOG_DEBUG("ChangeTabJob", "Executing job {} (direction: {})", job_id,
                  direction == TabDirection::NEXT ? "NEXT" : "PREVIOUS");

        // Single HTTP call with all commands batched
        std::string tab_command = (direction == TabDirection::NEXT) ? commands::NEXT_TAB : commands::PREVIOUS_TAB;

        std::vector<std::string> batch_commands = {
            tab_command,                           // Change tab
            commands::SET_OPERATION_GET_OPEN_TABS, // Set operation
            script_action_id,                      // Execute script action
            commands::GET_TABS,                    // Get tabs
            commands::GET_ACTIVE_INDEX,            // Get active index
            commands::TRANSPORT                    // Get transport state
        };

        std::string batch_url = buildCommandUrl(base_url, batch_commands);

        std::string response;
        int status_code;

        if (!network_mgr->httpGetBlocking(batch_url.c_str(), response, status_code) || status_code != 200)
        {
            LOG_ERROR("ChangeTabJob", "Batch request failed: status {}", status_code);
            return result;
        }

        // Parse batch response (newline-separated)
        auto lines = parseBatchResponse(response);
        if (lines.size() < 3)
        {
            LOG_ERROR("ChangeTabJob", "Invalid batch response - expected 3 lines, got {}", lines.size());
            return result;
        }

        // Parse transport state from last line (index 2)
        if (parseTransportState(lines[2], result->transport_state))
        {
            LOG_DEBUG("ChangeTabJob", "Successfully parsed transport state");
        }

        // Parse tabs from line 0 (GET_TABS response, 0-indexed so line 0)
        auto tab_items = parseTabSeparatedResponse(lines[0]);
        if (tab_items.size() >= 4 && tab_items[0] == commands::EXTSTATE_PREFIX &&
            tab_items[1] == commands::REAPER_SETLIST && tab_items[2] == "tabs")
        {
            std::string tab_data = tab_items[3];
            result->reaper_state.tabs = parseTabData(tab_data);
            LOG_DEBUG("ChangeTabJob", "Parsed {} tabs", result->reaper_state.tabs.size());
        }

        // Parse active index from line 1 (GET_ACTIVE_INDEX response, 0-indexed so line 1)
        auto index_items = parseTabSeparatedResponse(lines[1]);
        if (index_items.size() >= 4 && index_items[0] == commands::EXTSTATE_PREFIX &&
            index_items[1] == commands::REAPER_SETLIST && index_items[2] == "activeIndex")
        {
            try
            {
                result->reaper_state.active_index = std::stoi(index_items[3]);
                LOG_DEBUG("ChangeTabJob", "Got active index: {}", result->reaper_state.active_index);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR("ChangeTabJob", "Failed to parse active index: {}", e.what());
            }
        }

        result->success = true;
        result->reaper_state.success = true;
        LOG_DEBUG("ChangeTabJob", "Job {} completed successfully", job_id);

        return result;
    }

    // ChangePlaystateJob implementation
    std::unique_ptr<HttpJobResult> ChangePlaystateJob::execute(hal::INetworkManager *network_mgr, const std::string &base_url)
    {
        auto result = std::unique_ptr<ChangePlaystateResult>(new ChangePlaystateResult(job_id));

        LOG_DEBUG("ChangePlaystateJob", "Executing job {} (action: {})", job_id, static_cast<int>(action));

        // Single HTTP call with both commands batched
        std::string command;
        switch (action)
        {
        case PlayAction::PLAY:
            command = commands::PLAY;
            break;
        case PlayAction::STOP:
            command = commands::STOP;
            break;
        }

        std::vector<std::string> batch_commands = {
            command,            // Change playstate
            commands::TRANSPORT // Get transport state
        };

        std::string batch_url = buildCommandUrl(base_url, batch_commands);
        std::string response;
        int status_code;

        if (!network_mgr->httpGetBlocking(batch_url.c_str(), response, status_code) || status_code != 200)
        {
            LOG_ERROR("ChangePlaystateJob", "Batch request failed: status {}", status_code);
            return result;
        }

        // Parse batch response (newline-separated)
        auto lines = parseBatchResponse(response);
        if (lines.size() < 1)
        {
            LOG_ERROR("ChangePlaystateJob", "Invalid batch response - expected 1 lines, got {}", lines.size());
            return result;
        }

        // Parse transport state from first line (index 0)
        if (parseTransportState(lines[0], result->transport_state))
        {
            result->success = true;
            LOG_DEBUG("ChangePlaystateJob", "Successfully parsed transport state");
        }

        LOG_DEBUG("ChangePlaystateJob", "Job {} completed successfully", job_id);

        return result;
    }

    // GetStatusJob implementation
    std::unique_ptr<HttpJobResult> GetStatusJob::execute(hal::INetworkManager *network_mgr, const std::string &base_url)
    {
        auto result = std::unique_ptr<GetStatusResult>(new GetStatusResult(job_id));

        LOG_DEBUG("GetStatusJob", "Executing job {}", job_id);

        // Single HTTP call with all commands batched
        std::vector<std::string> batch_commands = {
            commands::SET_OPERATION_GET_OPEN_TABS, // Set operation
            script_action_id,                      // Execute script action
            commands::GET_TABS,                    // Get tabs
            commands::GET_ACTIVE_INDEX,            // Get active index
            commands::TRANSPORT                    // Get transport state
        };

        std::string batch_url = buildCommandUrl(base_url, batch_commands);
        std::string response;
        int status_code;

        if (!network_mgr->httpGetBlocking(batch_url.c_str(), response, status_code) || status_code != 200)
        {
            LOG_ERROR("GetStatusJob", "Batch request failed: status {}", status_code);
            return result;
        }

        // Parse batch response (newline-separated)
        auto lines = parseBatchResponse(response);
        if (lines.size() < 3)
        {
            LOG_ERROR("GetStatusJob", "Invalid batch response - expected 3 lines, got {}", lines.size());
            return result;
        }

        // Parse transport state from last line (index 2)
        if (parseTransportState(lines[2], result->transport_state))
        {
            LOG_DEBUG("GetStatusJob", "Successfully parsed transport state");
        }

        // Parse tabs from line 0 (GET_TABS response, 0-indexed so line 0)
        auto tab_items = parseTabSeparatedResponse(lines[0]);
        if (tab_items.size() >= 4 && tab_items[0] == commands::EXTSTATE_PREFIX &&
            tab_items[1] == commands::REAPER_SETLIST && tab_items[2] == "tabs")
        {
            std::string tab_data = tab_items[3];
            result->reaper_state.tabs = parseTabData(tab_data);
            LOG_DEBUG("GetStatusJob", "Parsed {} tabs", result->reaper_state.tabs.size());
        }

        // Parse active index from line 1 (GET_ACTIVE_INDEX response, 0-indexed so line 1)
        auto index_items = parseTabSeparatedResponse(lines[1]);
        if (index_items.size() >= 4 && index_items[0] == commands::EXTSTATE_PREFIX &&
            index_items[1] == commands::REAPER_SETLIST && index_items[2] == "activeIndex")
        {
            try
            {
                result->reaper_state.active_index = std::stoi(index_items[3]);
                LOG_DEBUG("GetStatusJob", "Got active index: {}", result->reaper_state.active_index);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR("GetStatusJob", "Failed to parse active index: {}", e.what());
            }
        }

        result->success = true;
        result->reaper_state.success = true;
        LOG_DEBUG("GetStatusJob", "Job {} completed successfully", job_id);

        return result;
    }

    // GetScriptActionIdJob implementation
    std::unique_ptr<HttpJobResult> GetScriptActionIdJob::execute(hal::INetworkManager *network_mgr, const std::string &base_url)
    {
        auto result = std::unique_ptr<GetScriptActionIdResult>(new GetScriptActionIdResult(job_id));

        LOG_DEBUG("GetScriptActionIdJob", "Executing job {}", job_id);

        // Get ReaperSetlist script action ID
        std::string command_url = buildCommandUrl(base_url, commands::GET_SCRIPT_ACTION_ID);

        std::string response;
        int status_code;

        if (!network_mgr->httpGetBlocking(command_url.c_str(), response, status_code) || status_code != 200)
        {
            LOG_ERROR("GetScriptActionIdJob", "Script action ID request failed: status {}", status_code);
            return result;
        }

        // Parse the response - expected format: "EXTSTATE\tReaperSetlist\tScriptActionId\t{actual_id}"
        auto items = parseTabSeparatedResponse(response);
        if (items.size() >= 4 && items[0] == commands::EXTSTATE_PREFIX && items[1] == commands::REAPER_SETLIST && items[2] == commands::SCRIPT_ACTION_ID_KEY)
        {
            result->script_action_id = items[3];
            result->success = true;
            LOG_INFO("GetScriptActionIdJob", "Got ReaperSetlist script action ID: {}", result->script_action_id);
        }
        else
        {
            LOG_ERROR("GetScriptActionIdJob", "Invalid script action ID response format");
        }

        LOG_DEBUG("GetScriptActionIdJob", "Job {} completed", job_id);

        return result;
    }

    std::unique_ptr<HttpJobResult> GetTransportJob::execute(hal::INetworkManager *network_mgr, const std::string &base_url)
    {
        auto result = std::unique_ptr<GetTransportResult>(new GetTransportResult(job_id));

        LOG_DEBUG("GetTransportJob", "Executing job {}", job_id);

        // Get transport state only
        std::string command_url = buildCommandUrl(base_url, commands::TRANSPORT);

        std::string response;
        int status_code;

        if (!network_mgr->httpGetBlocking(command_url.c_str(), response, status_code) || status_code != 200)
        {
            LOG_ERROR("GetTransportJob", "Transport request failed: status {}", status_code);
            return result;
        }

        // Parse the transport state using existing helper function
        if (parseTransportState(response, result->transport_state))
        {
            result->success = true;
            LOG_DEBUG("GetTransportJob", "Got transport state: play_state={}, position={:.2f}s",
                      result->transport_state.play_state, result->transport_state.position_seconds);
        }
        else
        {
            LOG_ERROR("GetTransportJob", "Failed to parse transport response");
        }

        LOG_DEBUG("GetTransportJob", "Job {} completed", job_id);

        return result;
    }

    // HttpJobManager implementation
    HttpJobManager::HttpJobManager(hal::ISystemHAL *system, const std::string &reaper_base_url)
        : system_hal(system), base_url(reaper_base_url), next_job_id(1), worker_running(false)
    {
#ifdef ARDUINO
        job_queue = nullptr;
        worker_task_handle = nullptr;
#else
        should_stop = false;
#endif
    }

    HttpJobManager::~HttpJobManager()
    {
        shutdown();
    }

    bool HttpJobManager::initialize()
    {
        LOG_INFO("HttpJobManager", "Initializing HTTP job manager");

#ifdef ARDUINO
        // Create FreeRTOS queue for jobs
        job_queue = xQueueCreate(JOB_QUEUE_SIZE, sizeof(HttpJob *));
        if (!job_queue)
        {
            LOG_ERROR("HttpJobManager", "Failed to create job queue");
            return false;
        }

        // Create FreeRTOS queue for results (main thread receives from this)
        main_result_queue = xQueueCreate(RESULT_QUEUE_SIZE, sizeof(HttpJobResult *));
        if (!main_result_queue)
        {
            LOG_ERROR("HttpJobManager", "Failed to create result queue");
            vQueueDelete(job_queue);
            job_queue = nullptr;
            return false;
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
            job_queue = nullptr;
            return false;
        }
#else
        // Create worker thread
        should_stop = false;
        worker_thread = std::thread(&HttpJobManager::workerThreadFunction, this);
#endif

        worker_running = true;

        // Submit script action ID job as the first job
        uint32_t script_job_id = submitGetScriptActionIdJob();
        LOG_INFO("HttpJobManager", "Submitted initial script action ID job {}", script_job_id);

        LOG_INFO("HttpJobManager", "HTTP job manager initialized successfully");
        return true;
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

                // Send result back
                sendResult(std::move(result));
            }
        }

        LOG_INFO("HttpJobManager", "Worker thread ended");
    }
#endif

} // namespace http
