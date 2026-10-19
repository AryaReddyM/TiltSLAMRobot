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
#include "tiltslam_base/math_functions.hpp"

using namespace std::chrono_literals;

class ESPComms : public rclcpp::Node {
public:
  ESPComms() : Node("esp_comms") {
    subscriber = this->create_subscription<geometry_msgs::msg::Twist>("/cmd_vel", 10, 
            std::bind(&ESPComms::VelCallback, this, std::placeholders::_1));
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
    pollfd p{fd, POLL_IN, 0};
    if (poll(&p, 1, 100) > 0) {
      char buf[256];
      ssize_t n = read(fd, buf, sizeof(buf));
      RCLCPP_INFO(this->get_logger(), "%.*s", (int)n, buf);
    }
  }

  void VelCallback(const geometry_msgs::msg::Twist::SharedPtr msg) {
    long speed = map(msg->linear.x, -512, 512, -255, 255);

    int angle = (int)msg->angular.z;
    int speedL = speed;
    int speedR = speed;

    if (angle < 0) { // Turning Left
      speedL -= map(angle, -512, 512, -100, 100);
    }
    else if (angle > 0) { // Turning Right
      speedR += map(angle, -512, 512, -100, 100);
    }

    // Deadzone
    if (abs(speedL) < 40) speedL = 0;
    if (abs(speedR) < 40) speedR = 0;

    std::string line = "V " + std::to_string(speedL) + " " + std::to_string(speedR) + "\n";
    write(fd, line.data(), line.size());
  }

public:
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

private:
  int fd;
  rclcpp::TimerBase::SharedPtr timer;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr subscriber;
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);

  auto node = std::make_shared<ESPComms>();
  RCLCPP_INFO(node->get_logger(), "ESPComms Init");

  rclcpp::spin(node);

  rclcpp::shutdown();
  return 0;
}
