#define WIN32_LEAN_AND_MEAN
#include "serial_port.h"

#include <iostream>

SerialPort::~SerialPort() { close(); }

bool SerialPort::open(const std::string& port, DWORD baud) {
    close();
    const std::string device = "\\\\.\\" + port;
    handle_ = CreateFileA(device.c_str(), GENERIC_READ | GENERIC_WRITE,
                          0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE) {
        std::cerr << "Cannot open " << port << " (Windows error " << GetLastError()
                  << "). Close Arduino Serial Monitor.\n";
        return false;
    }
    if (!SetupComm(handle_, 4096, 4096)) {
        std::cerr << "SetupComm failed: " << GetLastError() << "\n";
        close();
        return false;
    }
    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(handle_, &dcb)) {
        std::cerr << "GetCommState failed: " << GetLastError() << "\n";
        close();
        return false;
    }
    dcb.BaudRate = baud;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    if (!SetCommState(handle_, &dcb)) {
        std::cerr << "SetCommState failed: " << GetLastError() << "\n";
        close();
        return false;
    }
    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    if (!SetCommTimeouts(handle_, &timeouts)) {
        std::cerr << "SetCommTimeouts failed: " << GetLastError() << "\n";
        close();
        return false;
    }
    PurgeComm(handle_, PURGE_RXCLEAR | PURGE_TXCLEAR);
    return true;
}

bool SerialPort::sendMask(std::uint8_t mask) {
    if (handle_ == INVALID_HANDLE_VALUE || mask > 7) return false;
    const char message[] = {static_cast<char>('0' + mask), '\n'};
    DWORD written = 0;
    if (!WriteFile(handle_, message, sizeof(message), &written, nullptr) ||
        written != sizeof(message)) {
        std::cerr << "Serial write error: " << GetLastError() << "\n";
        return false;
    }
    return true;
}

void SerialPort::close() {
    if (handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
}
