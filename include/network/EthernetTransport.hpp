#pragma once

#include "devices/DeviceConfig.hpp"

#include <optional>
#include <string>

namespace network {

class EthernetTransport {
public:
    EthernetTransport();

    std::optional<double> readMeasurement(const devices::MeasurementDevice& device) const;
    bool writeActuator(const devices::ActuatorDevice& device, double value) const;

private:
    static std::optional<int> openSocket(const std::string& ip, std::uint16_t port);
};

} // namespace network
