#include "aws_iot/client_iot.hpp"

#include <aws/crt/Api.h>
#include <aws/crt/mqtt/Mqtt5Packets.h>
#include <aws/iot/Mqtt5Client.h>

#include <cassert>

#include "spdlog/spdlog.h"

namespace aws_iot
{
namespace iot = Aws::Iot;
namespace crt = Aws::Crt;
namespace mqtt5 = crt::Mqtt5;

class ClientIot::ClientIotImpl
{
public:
    ClientIotImpl(const std::string& endpoint,
                  const std::string& cert,
                  const std::string& key,
                  const std::string& ca,
                  const std::string& clientId,
                  const on_received_handler& handler)
        : m_on_received_handler(handler)
    {
        assert(handler);

        Aws::Iot::Mqtt5ClientBuilder* builder = Aws::Iot::Mqtt5ClientBuilder::NewMqtt5ClientBuilderWithMtlsFromPath(
            crt::String(endpoint.data(), endpoint.size()), cert.data(), key.data());
        if (builder == nullptr)
        {
            SPDLOG_CRITICAL("[iotb][aws_iot] failed to create mqtt5 client builder, terminating");
            exit(1);
        }

        if (!ca.empty())
        {
            builder->WithCertificateAuthority(ca.data());
        }
        std::shared_ptr<mqtt5::ConnectPacket> connectOptions = std::make_shared<mqtt5::ConnectPacket>();
        connectOptions->WithClientId(crt::String(clientId.data(), clientId.size()));
        builder->WithConnectOptions(connectOptions);

        builder->WithClientConnectionSuccessCallback([this](const mqtt5::OnConnectionSuccessEventData& eventData) {
            SPDLOG_DEBUG("[iotb][aws_iot] connected, client id: {}", eventData.negotiatedSettings->getClientId().c_str());
            try
            {
                m_connection_promise.set_value(true);
            }
            catch (...)
            {
                SPDLOG_ERROR("[iotb][aws_iot] exception ocurred trying to set connection promise (likely already set)");
            }
        });

        builder->WithClientConnectionFailureCallback([this](const mqtt5::OnConnectionFailureEventData& eventData) {
            SPDLOG_ERROR("[iotb][aws_iot] connection failed with error:", aws_error_debug_str(eventData.errorCode));
            try
            {
                m_connection_promise.set_value(false);
            }
            catch (...)
            {
                SPDLOG_ERROR("[iotb][aws_iot] exception ocurred trying to set connection promise (likely already set)");
            }
        });

        builder->WithClientStoppedCallback([this](const mqtt5::OnStoppedEventData&) {
            SPDLOG_DEBUG("[iotb][aws_iot] stopped");
            m_stopped_promise.set_value();
        });

        builder->WithClientDisconnectionCallback([this](const mqtt5::OnDisconnectionEventData& eventData) {
            SPDLOG_ERROR("[iotb][aws_iot] disconnection with error: {}", aws_error_debug_str(eventData.errorCode));
            if (eventData.disconnectPacket != nullptr)
            {
                SPDLOG_ERROR("[iotb][aws_iot] disconnection with reason code: {}", static_cast<uint32_t>(eventData.disconnectPacket->getReasonCode()));
            }
        });

        builder->WithPublishReceivedCallback([this](const mqtt5::PublishReceivedEventData& eventData) {
            SPDLOG_DEBUG("[iotb][aws_iot] receive a publish");

            std::lock_guard<std::mutex> lock(receiveMutex);
            if (eventData.publishPacket != nullptr)
            {
                SPDLOG_DEBUG("[iotb][aws_iot] publish received on topic: {}", eventData.publishPacket->getTopic().c_str());
                SPDLOG_TRACE("[iotb][aws_iot] message: {}", std::string_view(reinterpret_cast<char*>(eventData.publishPacket->getPayload().ptr), eventData.publishPacket->getPayload().len));

                for (mqtt5::UserProperty prop : eventData.publishPacket->getUserProperties())
                {
                    SPDLOG_DEBUG("[iotb][aws_iot] with user property: {} {}", prop.getName().c_str(), prop.getValue().c_str());
                }
                m_on_received_handler(eventData.publishPacket->getTopic().c_str(), {reinterpret_cast<char*>(eventData.publishPacket->getPayload().ptr),
                                                                                    eventData.publishPacket->getPayload().len});
            }
        });

        m_client = builder->Build();
        delete builder;
    }
    ~ClientIotImpl()
    {
        stop();
    }

    void start()
    {
        if (m_client->Start() && m_connection_promise.get_future().get())
        {
            SPDLOG_DEBUG("[iotb][aws_iot] connected");
        }
        else
        {
            SPDLOG_CRITICAL("[iotb][aws_iot] unable to connect, terminating");
            exit(1);
        }
    }

    void stop()
    {
        m_client->Stop();
    }

    void subscribe(std::string_view topic)
    {
        mqtt5::Subscription sub(topic.data(), AWS_MQTT5_QOS_AT_LEAST_ONCE);
        auto sub_packet = std::make_shared<mqtt5::SubscribePacket>();
        sub_packet->WithSubscription(std::move(sub));

        if (m_client->Subscribe(sub_packet))
        {
            SPDLOG_DEBUG("[iotb][aws_iot] successfully subscribed to topic: {}", topic);
        }
        else
        {
            SPDLOG_ERROR("[iotb][aws_iot] failed to subscribe to topic: {}", topic);
        }
    }

    void unsubscribe(std::string_view topic)
    {
        auto unsub_packet = std::make_shared<mqtt5::UnsubscribePacket>();
        unsub_packet->WithTopicFilter(topic.data());

        if (m_client->Unsubscribe(unsub_packet))
        {
            SPDLOG_DEBUG("[iotb][aws_iot] successfully unsubscribed from topic: {}", topic);
        }
        else
        {
            SPDLOG_ERROR("[iotb][aws_iot] failed to unsubscribe from topic: {}", topic);
        }
    }

    void publish(std::string_view topic, std::string_view payload)
    {
        auto publish_packet = std::make_shared<mqtt5::PublishPacket>();
        publish_packet->WithTopic(topic.data());
        // Cast justification: aws never modifies byte cursor inside of this setter, it just copies its content. No const correctness from aws side
        publish_packet->WithPayload({payload.size(), reinterpret_cast<uint8_t*>(const_cast<char*>(payload.data()))});
        publish_packet->WithQOS(AWS_MQTT5_QOS_AT_MOST_ONCE);

        if (m_client->Publish(std::move(publish_packet)))
        {
            SPDLOG_DEBUG("[iotb][aws_iot] successfully published to topic: {}", topic);
        }
        else
        {
            SPDLOG_ERROR("[iotb][aws_iot] failed to published to topic: {}", topic);
        }
    }

public:
    crt::ApiHandle m_handle;
    std::promise<bool> m_connection_promise;
    std::promise<void> m_stopped_promise;
    std::mutex receiveMutex;
    std::shared_ptr<mqtt5::Mqtt5Client> m_client;
    on_received_handler m_on_received_handler;
};

void ClientIot::connect()
{
    assert(!m_endpoint_path.empty());
    assert(!m_cert_path.empty());
    assert(!m_key_path.empty());
    assert(!m_ca_path.empty());
    assert(!m_client_id_path.empty());
    assert(m_handler);
    m_impl.reset(new ClientIotImpl(m_endpoint_path, m_cert_path, m_key_path, m_ca_path, m_client_id_path, m_handler));
    m_impl->start();
}
void ClientIot::disconnect()
{
    m_impl.reset();
}
void ClientIot::subscribe(std::string_view topic)
{
    assert(m_impl);
    m_impl->subscribe(topic);
}
void ClientIot::unsubscribe(std::string_view topic)
{
    assert(m_impl);
    m_impl->unsubscribe(topic);
}
void ClientIot::publish(std::string_view topic, std::string_view payload)
{
    assert(m_impl);
    m_impl->publish(topic, payload);
}
void ClientIot::setOnReceivedHandler(const on_received_handler& handler)
{
    m_handler = handler;
}
void ClientIot::setCredentials(std::string_view endpoint_path,
                               std::string_view cert_path,
                               std::string_view key_path,
                               std::string_view ca_path,
                               std::string_view client_id_path)
{
    m_endpoint_path = endpoint_path;
    m_cert_path = cert_path;
    m_key_path = key_path;
    m_ca_path = ca_path;
    m_client_id_path = client_id_path;
}

ClientIot::~ClientIot() = default;
}  // namespace aws_iot
