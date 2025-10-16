#include "network/EthernetTransport.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

namespace network {

EthernetTransport::EthernetTransport() = default;

std::optional<int> EthernetTransport::openSocket(const std::string& ip, std::uint16_t port)
{
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return std::nullopt;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        ::close(sock);
        return std::nullopt;
    }

    if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(sock);
        return std::nullopt;
    }

    return sock;
}

std::optional<double> EthernetTransport::readMeasurement(const devices::MeasurementDevice& device) const
{
    auto sock = openSocket(device.ipAddress, device.port);
    if (!sock) {
        return std::nullopt;
    }

    constexpr std::string_view command{"READ\n"};
    if (::send(*sock, command.data(), command.size(), 0) != static_cast<ssize_t>(command.size())) {
        ::close(*sock);
        return std::nullopt;
    }

    char buffer[256]{};
    const auto received = ::recv(*sock, buffer, sizeof(buffer) - 1, 0);
    ::close(*sock);
    if (received <= 0) {
        return std::nullopt;
    }

    buffer[received] = '\0';
    try {
        double value = std::stod(buffer);
        return device.scale * value + device.offset;
    } catch (...) {
        return std::nullopt;
    }
}

bool EthernetTransport::writeActuator(const devices::ActuatorDevice& device, double value) const
{
    auto sock = openSocket(device.ipAddress, device.port);
    if (!sock) {
        return false;
    }

    std::string command = "WRITE " + std::to_string(value) + "\n";
    const bool success = ::send(*sock, command.data(), command.size(), 0) == static_cast<ssize_t>(command.size());
    ::close(*sock);
    return success;
}

} // namespace network
