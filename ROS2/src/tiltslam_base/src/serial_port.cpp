#include "tiltslam_base/serial_port.hpp"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>

// Opens a serial port in raw mode at 115200 baud and returns the file descriptor.
// path should be the /dev/serial/by-id/... name, not /dev/ttyUSB0.
int openSerial(const std::string& path) {
  int fd = ::open(path.c_str(), O_RDWR | O_NOCTTY);  // O_NOCTTY: don't make it our terminal
  if (fd < 0) {
    throw std::runtime_error("open " + path + ": " + std::strerror(errno));
  }

  termios tty{};
  if (tcgetattr(fd, &tty) != 0) {
    ::close(fd);
    throw std::runtime_error(std::string("tcgetattr: ") + std::strerror(errno));
  }

  cfmakeraw(&tty);                  // no echo, no line buffering, no newline rewriting
  cfsetispeed(&tty, B115200);
  cfsetospeed(&tty, B115200);
  tty.c_cflag |= (CLOCAL | CREAD);  // ignore modem control lines, enable receiver
  tty.c_cc[VMIN] = 0;               // read() never blocks; returns whatever has arrived
  tty.c_cc[VTIME] = 0;

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    ::close(fd);
    throw std::runtime_error(std::string("tcsetattr: ") + std::strerror(errno));
  }

  // Opening the port resets the ESP32. Wait for it to boot,
  // then throw away its boot messages so they don't hit your parser.
  std::this_thread::sleep_for(std::chrono::seconds(2));
  tcflush(fd, TCIOFLUSH);

  return fd;
}