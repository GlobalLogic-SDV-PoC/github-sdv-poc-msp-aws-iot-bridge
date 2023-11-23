#include "aws_iot/client_iot_impl.hpp"

#include <aws/crt/Api.h>
#include <aws/crt/mqtt/Mqtt5Packets.h>
#include <aws/iot/Mqtt5Client.h>

#include <cassert>
#include <iotb/client_iot.hpp>
#include <iotb/context.hpp>
#include <memory>

namespace aws_iot
{
AwsClientIotImpl::AwsClientIotImpl(const std::string& endpoint,
                                   const std::string& cert,
                                   const std::string& key,
                                   const std::string& ca,
                                   const std::string& clientId,
                                   const std::shared_ptr<iotb::Context>& ctx,
                                   const onReceivedHandler& handler)
    : m_ctx(ctx)
    , m_on_received_handler(handler)

{
    std::unique_ptr<Aws::Iot::Mqtt5ClientBuilder> builder(Aws::Iot::Mqtt5ClientBuilder::NewMqtt5ClientBuilderWithMtlsFromPath(
        Aws::Crt::String(endpoint.data(), endpoint.size()), cert.data(), key.data()));
    if (builder == nullptr)
    {
        // SPDLOG_CRITICAL("failed to create Aws::Crt::Mqtt5 client builder, terminating");
        exit(1);
    }

    if (!ca.empty())
    {
        builder->WithCertificateAuthority(ca.data());
    }
    std::shared_ptr<Aws::Crt::Mqtt5::ConnectPacket> connectOptions = std::make_shared<Aws::Crt::Mqtt5::ConnectPacket>();
    connectOptions->WithClientId(Aws::Crt::String(clientId.data(), clientId.size()));
    builder->WithConnectOptions(connectOptions);

    builder->WithClientConnectionSuccessCallback([this](const Aws::Crt::Mqtt5::OnConnectionSuccessEventData& eventData)
                                                 {
            // SPDLOG_ERROR("connected, client id: {}", eventData.negotiatedSettings->getClientId().c_str());
            try
            {
                m_connection_promise.set_value(true);
            }
            catch (...)
            {
                // SPDLOG_ERROR("exception ocurred trying to set connection promise (likely already set)");
            } });

    builder->WithClientConnectionFailureCallback([this](const Aws::Crt::Mqtt5::OnConnectionFailureEventData& eventData)
                                                 {
            // SPDLOG_ERROR("connection failed with error:", aws_error_debug_str(eventData.errorCode));
            try
            {
                m_connection_promise.set_value(false);
            }
            catch (...)
            {
                // SPDLOG_ERROR("exception ocurred trying to set connection promise (likely already set)");
            } });

    builder->WithClientStoppedCallback([this](const Aws::Crt::Mqtt5::OnStoppedEventData&)
                                       {
            // SPDLOG_ERROR("stopped");
            m_stopped_promise.set_value(); });

    builder->WithClientDisconnectionCallback([this](const Aws::Crt::Mqtt5::OnDisconnectionEventData& eventData)
                                             {
            // SPDLOG_ERROR("disconnection with error: {}", aws_error_debug_str(eventData.errorCode));
            if (eventData.disconnectPacket != nullptr)
            {
                // SPDLOG_ERROR("disconnection with reason code: {}", static_cast<uint32_t>(eventData.disconnectPacket->getReasonCode()));
            } });

    builder->WithPublishReceivedCallback([this](const Aws::Crt::Mqtt5::PublishReceivedEventData& eventData)
                                         {
            // SPDLOG_ERROR("receive a publish");

            std::lock_guard<std::mutex> lock(receiveMutex);
            if (eventData.publishPacket != nullptr)
            {
                // SPDLOG_ERROR("publish received on topic: {}", eventData.publishPacket->getTopic().c_str());
                // SPDLOG_TRACE("message: {}", iotb::Span(reinterpret_cast<char*>(eventData.publishPacket->getPayload().ptr), eventData.publishPacket->getPayload().len));

                for (Aws::Crt::Mqtt5::UserProperty prop : eventData.publishPacket->getUserProperties())
                {
                    // SPDLOG_ERROR("with user property: {} {}", prop.getName().c_str(), prop.getValue().c_str());
                }
                auto topic = eventData.publishPacket->getTopic();
                auto& load = eventData.publishPacket->getPayload();
                m_on_received_handler({topic.data(), topic.size()}, {load.ptr,load.len});
            } });

    m_client = builder->Build();
    if (m_client->Start() && m_connection_promise.get_future().get())
    {
        // SPDLOG_ERROR("connected");
    }
    else
    {
        // SPDLOG_CRITICAL("unable to connect, terminating");
        exit(1);
    }
}
AwsClientIotImpl::~AwsClientIotImpl()
{
    m_client->Stop();
}

void AwsClientIotImpl::subscribe(iotb::Span topic)
{
    Aws::Crt::Mqtt5::Subscription sub({static_cast<char*>(topic.buf), topic.len}, AWS_MQTT5_QOS_AT_LEAST_ONCE);
    auto sub_packet = std::make_shared<Aws::Crt::Mqtt5::SubscribePacket>();
    sub_packet->WithSubscription(std::move(sub));

    if (m_client->Subscribe(sub_packet))
    {
        // SPDLOG_ERROR("successfully subscribed to topic: {}", topic);
    }
    else
    {
        // SPDLOG_ERROR("failed to subscribe to topic: {}", topic);
    }
}

void AwsClientIotImpl::unsubscribe(iotb::Span topic)
{
    auto unsub_packet = std::make_shared<Aws::Crt::Mqtt5::UnsubscribePacket>();
    unsub_packet->WithTopicFilter({static_cast<char*>(topic.buf), topic.len});

    if (m_client->Unsubscribe(unsub_packet))
    {
        // SPDLOG_ERROR("successfully unsubscribed from topic: {}", topic);
    }
    else
    {
        // SPDLOG_ERROR("failed to unsubscribe from topic: {}", topic);
    }
}

void AwsClientIotImpl::publish(iotb::Span topic, iotb::Span payload)
{
    auto publish_packet = std::make_shared<Aws::Crt::Mqtt5::PublishPacket>();
    publish_packet->WithTopic({static_cast<char*>(topic.buf), topic.len});
    publish_packet->WithPayload({payload.len, static_cast<uint8_t*>(payload.buf)});
    publish_packet->WithQOS(AWS_MQTT5_QOS_AT_MOST_ONCE);

    if (m_client->Publish(std::move(publish_packet)))
    {
        // SPDLOG_ERROR("successfully published to topic: {}", topic);
    }
    else
    {
        // SPDLOG_ERROR("failed to published to topic: {}", topic);
    }
}

}  // namespace aws_iot
