#include "aws_iot/client_iot_impl.hpp"

#include <aws/crt/Api.h>
#include <aws/crt/mqtt/Mqtt5Packets.h>
#include <aws/iot/Mqtt5Client.h>

#include <iotb/client_iot.hpp>
#include <iotb/context.hpp>
#include <memory>
#include <rclcpp/logging.hpp>

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
        RCLCPP_FATAL(m_ctx->node->get_logger(), "%s", "failed to create Aws::Crt::Mqtt5 client builder, terminating");
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
            RCLCPP_FATAL(m_ctx->node->get_logger(), "connected, client id: %s", eventData.negotiatedSettings->getClientId().c_str());
            try
            {
                m_connection_promise.set_value(true);
            }
            catch (...)
            {
                RCLCPP_FATAL(m_ctx->node->get_logger(), "%s","exception ocurred trying to set connection promise (likely already set)");
            } });

    builder->WithClientConnectionFailureCallback([this](const Aws::Crt::Mqtt5::OnConnectionFailureEventData& eventData)
                                                 {
            RCLCPP_FATAL(m_ctx->node->get_logger(), "connection failed with error: %s", aws_error_debug_str(eventData.errorCode));
            try
            {
                m_connection_promise.set_value(false);
            }
            catch (...)
            {
                RCLCPP_FATAL(m_ctx->node->get_logger(), "%s","exception ocurred trying to set connection promise (likely already set)");
            } });

    builder->WithClientStoppedCallback([this](const Aws::Crt::Mqtt5::OnStoppedEventData&)
                                       {
            RCLCPP_FATAL(m_ctx->node->get_logger(), "%s","stopped");
            m_stopped_promise.set_value(); });

    builder->WithClientDisconnectionCallback([this](const Aws::Crt::Mqtt5::OnDisconnectionEventData& eventData)
                                             {
            RCLCPP_FATAL(m_ctx->node->get_logger(), "disconnection with error: %s", aws_error_debug_str(eventData.errorCode));
            if (eventData.disconnectPacket != nullptr)
            {
                RCLCPP_FATAL(m_ctx->node->get_logger(), "disconnection with reason code: %d", static_cast<uint32_t>(eventData.disconnectPacket->getReasonCode()));
            } });

    builder->WithPublishReceivedCallback([this](const Aws::Crt::Mqtt5::PublishReceivedEventData& eventData)
                                         {
            RCLCPP_FATAL(m_ctx->node->get_logger(), "%s","receive a publish");

            std::lock_guard<std::mutex> lock(receiveMutex);
            if (eventData.publishPacket != nullptr)
            {
                RCLCPP_FATAL(m_ctx->node->get_logger(), "publish received on topic: %s", eventData.publishPacket->getTopic().c_str());
                RCLCPP_DEBUG(m_ctx->node->get_logger(),"message: %s", std::string(reinterpret_cast<char*>(eventData.publishPacket->getPayload().ptr), eventData.publishPacket->getPayload().len).c_str());

                for (Aws::Crt::Mqtt5::UserProperty prop : eventData.publishPacket->getUserProperties())
                {
                    RCLCPP_FATAL(m_ctx->node->get_logger(), "with user property: %s %s", prop.getName().c_str(), prop.getValue().c_str());
                }
                auto topic = eventData.publishPacket->getTopic();
                auto& load = eventData.publishPacket->getPayload();
                m_on_received_handler(std::string{topic.data(), topic.size()}, std::string{static_cast<char*>(static_cast<void*>(load.ptr)),load.len});
            } });

    m_client = builder->Build();
    if (m_client->Start() && m_connection_promise.get_future().get())
    {
        RCLCPP_FATAL(m_ctx->node->get_logger(), "%s", "connected");
    }
    else
    {
        RCLCPP_FATAL(m_ctx->node->get_logger(), "%s", "unable to connect, terminating");
        exit(1);
    }
}
AwsClientIotImpl::~AwsClientIotImpl()
{
    m_client->Stop();
}

void AwsClientIotImpl::subscribe(std::string topic)
{
    Aws::Crt::Mqtt5::Subscription sub({&topic[0], topic.size()}, AWS_MQTT5_QOS_AT_LEAST_ONCE);
    auto sub_packet = std::make_shared<Aws::Crt::Mqtt5::SubscribePacket>();
    sub_packet->WithSubscription(std::move(sub));

    if (m_client->Subscribe(sub_packet))
    {
        RCLCPP_FATAL(m_ctx->node->get_logger(), "successfully subscribed to topic: %s", topic.c_str());
    }
    else
    {
        RCLCPP_FATAL(m_ctx->node->get_logger(), "failed to subscribe to topic: %s", topic.c_str());
    }
}

void AwsClientIotImpl::unsubscribe(std::string topic)
{
    auto unsub_packet = std::make_shared<Aws::Crt::Mqtt5::UnsubscribePacket>();
    unsub_packet->WithTopicFilter({&topic[0], topic.size()});

    if (m_client->Unsubscribe(unsub_packet))
    {
        RCLCPP_FATAL(m_ctx->node->get_logger(), "successfully unsubscribed from topic: %s", topic.c_str());
    }
    else
    {
        RCLCPP_FATAL(m_ctx->node->get_logger(), "failed to unsubscribe from topic: %s", topic.c_str());
    }
}

void AwsClientIotImpl::publish(std::string topic, std::string payload)
{
    auto publish_packet = std::make_shared<Aws::Crt::Mqtt5::PublishPacket>();
    publish_packet->WithTopic({&topic[0], topic.size()});
    publish_packet->WithPayload({topic.size(), static_cast<uint8_t*>(static_cast<void*>(&topic[0]))});
    publish_packet->WithQOS(AWS_MQTT5_QOS_AT_MOST_ONCE);

    if (m_client->Publish(std::move(publish_packet)))
    {
        RCLCPP_FATAL(m_ctx->node->get_logger(), "successfully published to topic: %s", topic.c_str());
    }
    else
    {
        RCLCPP_FATAL(m_ctx->node->get_logger(), "failed to published to topic: %s", topic.c_str());
    }
}

}  // namespace aws_iot
