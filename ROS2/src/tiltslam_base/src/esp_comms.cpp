#include <cstdio>
#include <rclcpp/rclcpp.hpp>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <poll.h>
#include "geometry_msgs/msg/twist.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include <sstream>

using namespace std::chrono_literals;

class ESPComms : public rclcpp::Node {
public:
  ESPComms() : Node("esp_comms") {
    velSubscriber = this->create_subscription<geometry_msgs::msg::Twist>("/cmd_vel", 10, std::bind(&ESPComms::VelCallback, this, std::placeholders::_1));
    joyPublisher = this->create_publisher<sensor_msgs::msg::Joy>("joy", 10);
    timer = this->create_wall_timer(100ms, std::bind(&ESPComms::Tick, this));

    fd = openSerial("/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0");
  }

  ~ESPComms() {
    if (fd >= 0) {
      close(fd);
    }
  }

private:
  void Tick() {
    pollfd p{fd, POLLIN, 0};
    if (poll(&p, 1, 0) <= 0) return;
 
    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n <= 0) return;
 
    rxBuffer.append(buf, n);
 
    size_t pos;
    while ((pos = rxBuffer.find('\n')) != std::string::npos) {
      std::string line = rxBuffer.substr(0, pos);
      rxBuffer.erase(0, pos + 1);
      if (!line.empty() && line.back() == '\r') line.pop_back();
      HandleLine(line);
    }
  }

  void VelCallback(const geometry_msgs::msg::Twist::SharedPtr msg) {
    const double wheelRadius = 0.0325;
    const double trackWidth = 0.197;
    const double maxWheelSpeed = 13.6;

    double v = msg->linear.x;
    double w = msg->angular.z;

    double leftGround  = v - w * trackWidth / 2.0;
    double rightGround = v + w * trackWidth / 2.0;

    double left  = leftGround  / wheelRadius;
    double right = rightGround / wheelRadius;

    double biggest = std::max(std::abs(left), std::abs(right));
    if (biggest > maxWheelSpeed) {
      double scale = maxWheelSpeed / biggest;
      left *= scale;
      right *= scale;
    }

    std::string line = "V " + std::to_string(left) + " " + std::to_string(right) + "\n";
    write(fd, line.data(), line.size());
  }

  int openSerial(const std::string& path) {
    int fd = ::open(path.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0) {
      throw std::runtime_error("open " + path + ": " + std::strerror(errno));
    }

    termios tty{};
    if (tcgetattr(fd, &tty) != 0) {
      ::close(fd);
      throw std::runtime_error(std::string("tcgetattr: ") + std::strerror(errno));
    }

    cfmakeraw(&tty);
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
      ::close(fd);
      throw std::runtime_error(std::string("tcsetattr: ") + std::strerror(errno));
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    tcflush(fd, TCIOFLUSH);

    return fd;
  }

  void HandleLine(const std::string& line) {
    std::istringstream in(line);
    std::string type;
    in >> type;
 
    if (type == "J") {
      float leftY, rightX;
      if (!(in >> leftY >> rightX)) return; 
 
      sensor_msgs::msg::Joy msg;
      msg.header.stamp = this->now();
      msg.axes = {0.0f, leftY, rightX, 0.0f};
      joyPublisher->publish(msg);
    }
  }

  int fd = -1;
  std::string rxBuffer;
  rclcpp::TimerBase::SharedPtr timer;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr velSubscriber;
  rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr joyPublisher;
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);

  auto node = std::make_shared<ESPComms>();
  RCLCPP_INFO(node->get_logger(), "ESPComms Init");

  rclcpp::spin(node);

  rclcpp::shutdown();
  return 0;
}
