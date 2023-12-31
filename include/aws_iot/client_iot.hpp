#pragma once

#include <memory>
#include <string>

#include "iotb/interface/client_iot.hpp"

namespace aws_iot
{
class ClientIot : public iotb::IClientIot
{
    class ClientIotImpl;

public:
    void connect() override;
    void disconnect() override;
    void subscribe(std::string_view topic) override;
    void unsubscribe(std::string_view topic) override;
    void publish(std::string_view topic, std::string_view payload) override;
    void setOnReceivedHandler(const on_received_handler& handler) override;
    void setConfig(const nlohmann::json& config) override;
    ~ClientIot();
    ClientIot();
private:
    std::string m_endpoint_path;
    std::string m_cert_path;
    std::string m_key_path;
    std::string m_ca_path;
    std::string m_client_id_path;
    on_received_handler m_handler;
    
    std::unique_ptr<ClientIotImpl> m_impl;
};
}  // namespace aws_iot