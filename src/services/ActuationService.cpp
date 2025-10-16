#include "services/ActuationService.hpp"

#include "devices/DeviceRegistry.hpp"
#include "network/EthernetTransport.hpp"

#include <iostream>

namespace services {

ActuationService::ActuationService(devices::DeviceRegistry& registry, const network::EthernetTransport& transport)
    : registry_(registry)
    , transport_(transport)
{
}

bool ActuationService::writeValue(const std::string& id, double value, WriteSource)
{
    auto device = registry_.findActuator(id);
    if (!device) {
        return false;
    }
    if (!validateValue(*device, value)) {
        return false;
    }

    std::scoped_lock lock(mutex_);
    return transport_.writeActuator(*device, value);
}

std::optional<devices::ActuatorDevice> ActuationService::findActuator(const std::string& id) const
{
    return registry_.findActuator(id);
}

bool ActuationService::validateValue(const devices::ActuatorDevice& device, double value) const
{
    if (value < device.minimum || value > device.maximum) {
        std::cerr << "Value for actuator " << device.id << " out of range" << std::endl;
        return false;
    }
    return true;
}

} // namespace services
