
#include "aws_iot/client_iot.hpp"

#include <memory>

#include "aws_iot/client_iot_impl.hpp"

namespace aws_iot
{
void AwsClientIot::connect(const std::shared_ptr<iotb::Context>& ctx, const nlohmann::json& config, const onReceivedHandler& rec)
{
    m_impl = std::make_shared<AwsClientIotImpl>(config["endpoint"], config["certificate"], config["private"], config["root"], config["clientId"], ctx, rec);
}
void AwsClientIot::disconnect()
{
    m_impl.reset();
}
void AwsClientIot::subscribe(std::string topic)
{
    m_impl->subscribe(topic);
}
void AwsClientIot::unsubscribe(std::string topic)
{
    m_impl->unsubscribe(topic);
}
void AwsClientIot::publish(std::string topic, std::string payload)
{
    m_impl->publish(topic, payload);
}
}  // namespace aws_iot