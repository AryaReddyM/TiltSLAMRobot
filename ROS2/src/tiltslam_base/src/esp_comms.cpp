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
#include "poll.h"

using namespace std::chrono_literals;

int fd;

class ESPComms : public rclcpp::Node {
public:
  ESPComms() : Node("esp_comms") {
    timer = this->create_wall_timer(100ms, std::bind(&ESPComms::Tick, this));

    fd = openSerial("/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0");
  }

private:
  void Tick() {
    std::string line = "V 1.5 -0.5\n";
    write(fd, line.data(), line.size());

    pollfd p{fd, POLL_IN, 0};
    if (poll(&p, 1, 100) > 0) {
      char buf[256];
      ssize_t n = read(fd, buf, sizeof(buf));
      RCLCPP_INFO(this->get_logger(), "%.*s", (int)n, buf);
    }
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
  rclcpp::TimerBase::SharedPtr timer;
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);

  auto node = std::make_shared<ESPComms>();
  RCLCPP_INFO(node->get_logger(), "ESPComms Init");

  rclcpp::spin(node);

  close(fd);
  rclcpp::shutdown();
  return 0;
}
