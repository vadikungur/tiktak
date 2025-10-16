#include "services/AcquisitionService.hpp"

#include "devices/DeviceRegistry.hpp"
#include "network/EthernetTransport.hpp"

#include <chrono>

namespace services {

AcquisitionService::AcquisitionService(devices::DeviceRegistry& registry, const network::EthernetTransport& transport)
    : registry_(registry)
    , transport_(transport)
{
}

AcquisitionService::~AcquisitionService()
{
    stop();
}

void AcquisitionService::start()
{
    if (running_.exchange(true)) {
        return;
    }

    auto devices = registry_.measurementDevices();
    for (const auto& device : devices) {
        workers_.emplace_back([this, device]() { runDevice(device); });
    }
}

void AcquisitionService::stop()
{
    if (!running_.exchange(false)) {
        return;
    }
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
}

void AcquisitionService::setCallback(ValueCallback callback)
{
    callback_ = std::move(callback);
}

std::optional<devices::DeviceState> AcquisitionService::latestState(const std::string& id) const
{
    std::scoped_lock lock(stateMutex_);
    if (auto it = states_.find(id); it != states_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::unordered_map<std::string, devices::DeviceState> AcquisitionService::snapshot() const
{
    std::scoped_lock lock(stateMutex_);
    return states_;
}

void AcquisitionService::runDevice(const devices::MeasurementDevice& device)
{
    while (running_) {
        auto value = transport_.readMeasurement(device);
        if (value) {
            devices::DeviceState state{};
            state.value = *value;
            state.timestamp = std::chrono::system_clock::now();
            {
                std::scoped_lock lock(stateMutex_);
                states_[device.id] = state;
            }
            if (callback_) {
                callback_(device.id, state);
            }
        }
        std::this_thread::sleep_for(device.pollInterval);
    }
}

} // namespace services
