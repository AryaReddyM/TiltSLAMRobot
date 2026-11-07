#include <rclcpp/rclcpp.hpp>
#include "sensor_msgs/msg/joy.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include <cmath>

using namespace std::chrono_literals;

class Teleop : public rclcpp::Node {
public:
    Teleop() : Node("teleop") {
        joySubscriber = this->create_subscription<sensor_msgs::msg::Joy>("joy", 10, std::bind(&Teleop::JoyCallback, this, std::placeholders::_1));
        velPublisher = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
    }

    void JoyCallback(const sensor_msgs::msg::Joy::SharedPtr msg) {
        if (msg->axes.size() < 3) return;

        const float maxLinear = 0.3;
        const float maxAngular = 1.5;
        const float deadzone = 0.1;

        float stickY = msg->axes[1];
        float stickX = msg->axes[2];

        // Deadzone
        if (std::abs(stickY) < deadzone) stickY = 0;
        if (std::abs(stickX) < deadzone) stickX = 0;

        geometry_msgs::msg::Twist velMsg;
        velMsg.linear.x = stickY * maxLinear;
        velMsg.angular.z = stickX * maxAngular;

        velPublisher->publish(velMsg);
    }

    rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joySubscriber;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr velPublisher;
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);

  auto node = std::make_shared<Teleop>();
  RCLCPP_INFO(node->get_logger(), "Teleop Init");

  rclcpp::spin(node);

  rclcpp::shutdown();
  return 0;
}