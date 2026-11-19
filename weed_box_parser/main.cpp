#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "sectors.h"
#include "spray_zone.h"

#pragma comment(lib, "Ws2_32.lib")

namespace {
constexpr unsigned short UDP_PORT = 5005;
constexpr int MAX_BOXES = 32;

struct Config {
    int sectorCount = 3;
    double sectorOverlapPercent = 5.0;
    double zoneTopPercent = 40.0;
    double zoneBottomPercent = 60.0;
};

struct Box { int x = 0, y = 0, width = 0, height = 0; };

bool loadConfig(const std::string& filename, Config& cfg) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Cannot open " << filename << "\n";
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        const auto hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        auto trim = [](std::string s) {
            const auto start = s.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) return std::string{};
            const auto end = s.find_last_not_of(" \t\r\n");
            return s.substr(start, end - start + 1);
        };
        const std::string key = trim(line.substr(0, eq));
        const std::string value = trim(line.substr(eq + 1));
        try {
            std::size_t used = 0;
            if (key == "sector_count") {
                int n = std::stoi(value, &used);
                if (used != value.size()) return false;
                cfg.sectorCount = n;
            } else if (key == "sector_overlap_percent") {
                double v = std::stod(value, &used);
                if (used != value.size()) return false;
                cfg.sectorOverlapPercent = v;
            } else if (key == "spray_zone_top_percent") {
                double v = std::stod(value, &used);
                if (used != value.size()) return false;
                cfg.zoneTopPercent = v;
            } else if (key == "spray_zone_bottom_percent") {
                double v = std::stod(value, &used);
                if (used != value.size()) return false;
                cfg.zoneBottomPercent = v;
            }
        } catch (...) {
            std::cerr << "Invalid config value for " << key << "\n";
            return false;
        }
    }
    if (cfg.sectorCount < 1 || cfg.sectorCount > 16 ||
        !(cfg.sectorOverlapPercent >= 0 && cfg.sectorOverlapPercent <= 100) ||
        !(cfg.zoneTopPercent >= 0 && cfg.zoneTopPercent < cfg.zoneBottomPercent &&
          cfg.zoneBottomPercent <= 100)) {
        std::cerr << "Invalid settings: sectors=1..16, overlap=0..100, 0<=top<bottom<=100\n";
        return false;
    }
    return true;
}

bool parseFrame(const std::string& packet, int& width, int& height,
                std::vector<Box>& boxes) {
    std::istringstream in(packet);
    std::string tag;
    int count = 0;
    if (!(in >> tag >> width >> height >> count) || tag != "FRAME" ||
        width <= 0 || height <= 0 || width > 8192 || height > 8192 ||
        count < 0 || count > MAX_BOXES) return false;

    boxes.clear();
    for (int i = 0; i < count; ++i) {
        Box b;
        if (!(in >> b.x >> b.y >> b.width >> b.height) ||
            b.width <= 0 || b.height <= 0 || b.x < 0 || b.y < 0 ||
            static_cast<std::int64_t>(b.x) + b.width > width ||
            static_cast<std::int64_t>(b.y) + b.height > height) return false;
        boxes.push_back(b);
    }
    std::string extra;
    return !(in >> extra);
}

std::string binaryMask(std::uint32_t mask, int count) {
    std::string s;
    for (int i = count - 1; i >= 0; --i)
        s += (mask & (std::uint32_t{1} << i)) ? '1' : '0';
    return s;
}
}

int main() {
    Config cfg;
    if (!loadConfig("spray_config.txt", cfg)) return 1;

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
    SOCKET server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(UDP_PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "UDP bind failed (port in use?): " << WSAGetLastError() << "\n";
        closesocket(server);
        WSACleanup();
        return 1;
    }

    DWORD timeoutMs = 500;
    setsockopt(server, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));

    std::cout << "C++ UDP bridge on 127.0.0.1:" << UDP_PORT << "\n"
              << "Sectors: " << cfg.sectorCount << ", overlap: "
              << cfg.sectorOverlapPercent << "%\n"
              << "Spray zone: " << cfg.zoneTopPercent << "-"
              << cfg.zoneBottomPercent << "%\n"
              << "Waiting for Python... (Ctrl+C to stop)\n";

    std::uint32_t previousMask = 0;
    bool connectionActive = false;
    while (true) {
        char buffer[4096];
        sockaddr_in sender{};
        int senderLen = sizeof(sender);
        int size = recvfrom(server, buffer, sizeof(buffer), 0,
                            reinterpret_cast<sockaddr*>(&sender), &senderLen);
        if (size == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAETIMEDOUT) {
                if (connectionActive) {
                    std::cout << "Python timeout: mask -> 0 (safe state)\n";
                    previousMask = 0;
                    connectionActive = false;
                }
                continue;
            }
            std::cerr << "recvfrom failed: " << WSAGetLastError() << "\n";
            break;
        }
        // Local-only control channel.
        if (sender.sin_addr.s_addr != htonl(INADDR_LOOPBACK)) continue;

        int frameWidth = 0, frameHeight = 0;
        std::vector<Box> boxes;
        std::uint32_t mask = 0;
        if (parseFrame(std::string(buffer, buffer + size), frameWidth, frameHeight, boxes)) {
            for (const Box& b : boxes) {
                if (isInSprayZone(frameHeight, b.y, b.height,
                                  cfg.zoneTopPercent, cfg.zoneBottomPercent)) {
                    mask |= getSectorMask(frameWidth, b.x, b.y, b.width, b.height,
                                          cfg.sectorOverlapPercent, cfg.sectorCount);
                }
            }
            if (!connectionActive || mask != previousMask) {
                std::cout << "Objects: " << boxes.size() << " | mask: "
                          << binaryMask(mask, cfg.sectorCount) << " (" << mask << ")\n";
            }
            connectionActive = true;
            previousMask = mask;
        } else {
            // Invalid input means no actuation; do not trust stale coordinates.
            if (previousMask != 0) std::cout << "Invalid frame: mask -> 0\n";
            previousMask = 0;
        }

        std::ostringstream out;
        out << std::setprecision(8) << "STATE " << cfg.sectorCount << ' '
            << cfg.zoneTopPercent << ' ' << cfg.zoneBottomPercent << ' '
            << cfg.sectorOverlapPercent << ' ' << mask;
        const std::string reply = out.str();
        sendto(server, reply.data(), static_cast<int>(reply.size()), 0,
               reinterpret_cast<sockaddr*>(&sender), senderLen);
    }

    closesocket(server);
    WSACleanup();
    return 0;
}
