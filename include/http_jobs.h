#pragma once

#include "hal_interfaces.h"
#include "reaper_types.h"
#include "network_manager.h"
#include <string>
#include <memory>
#include <cstdint>

namespace http
{
    // Forward declarations
    class HttpJobResult;

    // Result type enumeration for type identification without RTTI
    enum class ResultType
    {
        WIFI_CONNECT,
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

    // WiFi Connection Job and Result
    class WiFiConnectResult : public HttpJobResult
    {
    public:
        bool connected;
        std::string ip_address;

        WiFiConnectResult(uint32_t id) : HttpJobResult(id, ResultType::WIFI_CONNECT), connected(false) {}
    };

    class WiFiConnectJob : public HttpJob
    {
    private:
        NetworkManager *network_manager;

    public:
        WiFiConnectJob(uint32_t id, NetworkManager *network);

        std::unique_ptr<HttpJobResult> execute(hal::INetworkManager *network_mgr, const std::string &base_url) override;
        const char *getJobTypeName() const override { return "WiFiConnect"; }
    }; // Change Tab Job and Result
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

    // Get Script Action ID Job and Result
    class GetScriptActionIdResult : public HttpJobResult
    {
    public:
        std::string script_action_id;

        GetScriptActionIdResult(uint32_t id) : HttpJobResult(id, ResultType::GET_SCRIPT_ACTION_ID) {}
    };

    class GetScriptActionIdJob : public HttpJob
    {
    public:
        GetScriptActionIdJob(uint32_t id) : HttpJob(id) {}

        std::unique_ptr<HttpJobResult> execute(hal::INetworkManager *network_mgr, const std::string &base_url) override;
        const char *getJobTypeName() const override { return "GetScriptActionId"; }
    };

    // Get Transport Job and Result
    class GetTransportResult : public HttpJobResult
    {
    public:
        reaper::TransportState transport_state;

        GetTransportResult(uint32_t id) : HttpJobResult(id, ResultType::GET_TRANSPORT)
        {
            transport_state = reaper::TransportState{};
        }
    };

    class GetTransportJob : public HttpJob
    {
    public:
        GetTransportJob(uint32_t id) : HttpJob(id) {}

        std::unique_ptr<HttpJobResult> execute(hal::INetworkManager *network_mgr, const std::string &base_url) override;
        const char *getJobTypeName() const override { return "GetTransport"; }
    };

} // namespace http
