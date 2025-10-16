#pragma once

#include "devices/DeviceConfig.hpp"

#include <filesystem>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

namespace devices {

class DeviceRegistry {
public:
    explicit DeviceRegistry(std::filesystem::path configPath);

    bool load();
    bool save();

    std::vector<MeasurementDevice> measurementDevices() const;
    std::vector<ActuatorDevice> actuatorDevices() const;

    std::optional<MeasurementDevice> findMeasurement(std::string_view id) const;
    std::optional<ActuatorDevice> findActuator(std::string_view id) const;

    bool updateMeasurement(const MeasurementDevice& device);
    bool updateActuator(const ActuatorDevice& device);

    bool addMeasurement(const MeasurementDevice& device);
    bool addActuator(const ActuatorDevice& device);

    bool removeMeasurement(std::string_view id);
    bool removeActuator(std::string_view id);

    const std::filesystem::path& configPath() const { return configPath_; }

private:
    bool parseLine(std::string_view line, std::string& currentSection, MeasurementDevice& measurementBuffer,
                   ActuatorDevice& actuatorBuffer, bool& measurementActive, bool& actuatorActive);
    void commitBuffers(const std::string& section, MeasurementDevice& measurementBuffer,
                       ActuatorDevice& actuatorBuffer, bool measurementActive, bool actuatorActive);
    static std::string makeSectionName(std::string_view kind, std::string_view id);

    std::filesystem::path configPath_;
    mutable std::shared_mutex mutex_;
    MeasurementMap measurements_;
    ActuatorMap actuators_;
};

} // namespace devices
