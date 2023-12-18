#pragma once

#include <aws/crt/Api.h>
#include <aws/crt/mqtt/Mqtt5Packets.h>
#include <aws/iot/Mqtt5Client.h>

#include <cassert>
#include <iotb/client_iot.hpp>
#include <iotb/context.hpp>
#include <memory>

namespace aws_iot
{
class AwsClientIotImpl
{
    using onReceivedHandler = iotb::IClientIot::onReceivedHandler;

public:
    AwsClientIotImpl(const std::string& endpoint,
                  const std::string& cert,
                  const std::string& key,
                  const std::string& ca,
                  const std::string& clientId,
                  const std::shared_ptr<iotb::Context>& ctx,
                  const onReceivedHandler& handler);
    ~AwsClientIotImpl();
    void subscribe(std::string topic);
    void unsubscribe(std::string topic);
    void publish(std::string topic, std::string payload);

protected:
    Aws::Crt::ApiHandle m_handle;
    std::shared_ptr<iotb::Context> m_ctx;
    std::promise<bool> m_connection_promise;
    std::promise<void> m_stopped_promise;
    std::mutex receiveMutex;
    std::shared_ptr<Aws::Crt::Mqtt5::Mqtt5Client> m_client;
    onReceivedHandler m_on_received_handler;
};
}  // namespace aws_iot