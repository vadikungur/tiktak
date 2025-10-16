#pragma once

#include "devices/DeviceConfig.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace devices {
class DeviceRegistry;
}

namespace network {
class EthernetTransport;
}

namespace services {

class AcquisitionService {
public:
    using ValueCallback = std::function<void(const std::string& id, const devices::DeviceState& state)>;

    AcquisitionService(devices::DeviceRegistry& registry, const network::EthernetTransport& transport);
    ~AcquisitionService();

    void start();
    void stop();

    void setCallback(ValueCallback callback);

    std::optional<devices::DeviceState> latestState(const std::string& id) const;
    std::unordered_map<std::string, devices::DeviceState> snapshot() const;

private:
    void runDevice(const devices::MeasurementDevice& device);
    void refreshWorkers();

    devices::DeviceRegistry& registry_;
    const network::EthernetTransport& transport_;
    std::atomic<bool> running_{false};
    mutable std::mutex stateMutex_;
    std::unordered_map<std::string, devices::DeviceState> states_;
    std::vector<std::thread> workers_;
    ValueCallback callback_;
};

} // namespace services
