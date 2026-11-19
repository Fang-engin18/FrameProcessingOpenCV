#pragma once

#include <cstdint>
#include <string>
#include <windows.h>

class SerialPort {
public:
    SerialPort() = default;
    ~SerialPort();
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    bool open(const std::string& port, DWORD baud = 115200);
    bool sendMask(std::uint8_t mask);
    void close();

private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
};
