#include <memory>

#include "aws_iot/client_iot.hpp"
#include "iotb/app.hpp"

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    auto iot_client = std::make_shared<aws_iot::AwsClientIot>();
    iotb::App app(iot_client);
    app.start();

    return 0;
}