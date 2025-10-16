#include "devices/DeviceConfig.hpp"

#include <stdexcept>
#include <string>

namespace devices {

ChannelType channelFromString(std::string_view value)
{
    if (value == "analog") {
        return ChannelType::Analog;
    }
    if (value == "digital") {
        return ChannelType::Digital;
    }
    throw std::invalid_argument("Unknown channel type: " + std::string(value));
}

std::string channelToString(ChannelType type)
{
    switch (type) {
    case ChannelType::Analog:
        return "analog";
    case ChannelType::Digital:
        return "digital";
    }
    return "unknown";
}

} // namespace devices
