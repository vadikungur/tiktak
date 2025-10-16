#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace devices {

enum class ChannelType {
    Analog,
    Digital
};

struct MeasurementDevice {
    std::string id;
    std::string name;
    ChannelType channel;
    std::string ipAddress;
    std::uint16_t port{0};
    std::string driver;
    double scale{1.0};
    double offset{0.0};
    std::chrono::milliseconds pollInterval{1000};
};

struct ActuatorDevice {
    std::string id;
    std::string name;
    ChannelType channel;
    std::string ipAddress;
    std::uint16_t port{0};
    std::string driver;
    double minimum{0.0};
    double maximum{1.0};
    double defaultValue{0.0};
};

struct DeviceState {
    double value{0.0};
    std::chrono::system_clock::time_point timestamp{};
};

using MeasurementMap = std::unordered_map<std::string, MeasurementDevice>;
using ActuatorMap = std::unordered_map<std::string, ActuatorDevice>;
using DeviceStateMap = std::unordered_map<std::string, DeviceState>;

ChannelType channelFromString(std::string_view value);
std::string channelToString(ChannelType type);

} // namespace devices
