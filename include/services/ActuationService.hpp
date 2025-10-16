#pragma once

#include "devices/DeviceConfig.hpp"

#include <mutex>
#include <optional>
#include <string>

namespace devices {
class DeviceRegistry;
}

namespace network {
class EthernetTransport;
}

namespace services {

class ActuationService {
public:
    enum class WriteSource {
        Manual,
        Algorithm
    };

    ActuationService(devices::DeviceRegistry& registry, const network::EthernetTransport& transport);

    bool writeValue(const std::string& id, double value, WriteSource source);
    std::optional<devices::ActuatorDevice> findActuator(const std::string& id) const;

private:
    bool validateValue(const devices::ActuatorDevice& device, double value) const;

    devices::DeviceRegistry& registry_;
    const network::EthernetTransport& transport_;
    mutable std::mutex mutex_;
};

} // namespace services
