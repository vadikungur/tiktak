#include "devices/DeviceRegistry.hpp"

#include <fstream>
#include <sstream>

namespace {
constexpr std::string_view measurementPrefix{"measurement:"};
constexpr std::string_view actuatorPrefix{"actuator:"};
}

namespace devices {

DeviceRegistry::DeviceRegistry(std::filesystem::path configPath)
    : configPath_(std::move(configPath))
{
}

bool DeviceRegistry::load()
{
    std::ifstream input(configPath_);
    if (!input.is_open()) {
        return false;
    }

    MeasurementMap measurements;
    ActuatorMap actuators;

    std::string line;
    std::string currentSection;
    MeasurementDevice measurementBuffer;
    ActuatorDevice actuatorBuffer;
    bool measurementActive = false;
    bool actuatorActive = false;

    auto commit = [&](MeasurementMap& targetMeasurements, ActuatorMap& targetActuators) {
        if (currentSection.starts_with(measurementPrefix) && measurementActive) {
            MeasurementDevice device = measurementBuffer;
            device.id = currentSection.substr(measurementPrefix.size());
            targetMeasurements[device.id] = std::move(device);
            measurementBuffer = MeasurementDevice{};
        } else if (currentSection.starts_with(actuatorPrefix) && actuatorActive) {
            ActuatorDevice device = actuatorBuffer;
            device.id = currentSection.substr(actuatorPrefix.size());
            targetActuators[device.id] = std::move(device);
            actuatorBuffer = ActuatorDevice{};
        }
        measurementActive = false;
        actuatorActive = false;
    };

    while (std::getline(input, line)) {
        const bool isSection = parseLine(line, currentSection, measurementBuffer, actuatorBuffer, measurementActive, actuatorActive);
        if (isSection) {
            commit(measurements, actuators);
            std::string_view section(line.data() + 1, line.size() - 2);
            currentSection = std::string(section);
        }
    }

    commit(measurements, actuators);

    {
        std::unique_lock lock(mutex_);
        measurements_ = std::move(measurements);
        actuators_ = std::move(actuators);
    }

    return true;
}

bool DeviceRegistry::save()
{
    std::ofstream output(configPath_);
    if (!output.is_open()) {
        return false;
    }

    std::shared_lock lock(mutex_);

    for (const auto& [id, device] : measurements_) {
        output << '[' << makeSectionName("measurement", id) << "]\n";
        output << "name=" << device.name << "\n";
        output << "channel=" << channelToString(device.channel) << "\n";
        output << "ip=" << device.ipAddress << "\n";
        output << "port=" << device.port << "\n";
        output << "driver=" << device.driver << "\n";
        output << "scale=" << device.scale << "\n";
        output << "offset=" << device.offset << "\n";
        output << "poll_interval_ms=" << device.pollInterval.count() << "\n\n";
    }

    for (const auto& [id, device] : actuators_) {
        output << '[' << makeSectionName("actuator", id) << "]\n";
        output << "name=" << device.name << "\n";
        output << "channel=" << channelToString(device.channel) << "\n";
        output << "ip=" << device.ipAddress << "\n";
        output << "port=" << device.port << "\n";
        output << "driver=" << device.driver << "\n";
        output << "min=" << device.minimum << "\n";
        output << "max=" << device.maximum << "\n";
        output << "default=" << device.defaultValue << "\n\n";
    }

    return true;
}

std::vector<MeasurementDevice> DeviceRegistry::measurementDevices() const
{
    std::shared_lock lock(mutex_);
    std::vector<MeasurementDevice> result;
    result.reserve(measurements_.size());
    for (const auto& [_, device] : measurements_) {
        result.push_back(device);
    }
    return result;
}

std::vector<ActuatorDevice> DeviceRegistry::actuatorDevices() const
{
    std::shared_lock lock(mutex_);
    std::vector<ActuatorDevice> result;
    result.reserve(actuators_.size());
    for (const auto& [_, device] : actuators_) {
        result.push_back(device);
    }
    return result;
}

std::optional<MeasurementDevice> DeviceRegistry::findMeasurement(std::string_view id) const
{
    std::shared_lock lock(mutex_);
    if (auto it = measurements_.find(std::string(id)); it != measurements_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<ActuatorDevice> DeviceRegistry::findActuator(std::string_view id) const
{
    std::shared_lock lock(mutex_);
    if (auto it = actuators_.find(std::string(id)); it != actuators_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool DeviceRegistry::updateMeasurement(const MeasurementDevice& device)
{
    std::unique_lock lock(mutex_);
    auto it = measurements_.find(device.id);
    if (it == measurements_.end()) {
        return false;
    }
    it->second = device;
    return true;
}

bool DeviceRegistry::updateActuator(const ActuatorDevice& device)
{
    std::unique_lock lock(mutex_);
    auto it = actuators_.find(device.id);
    if (it == actuators_.end()) {
        return false;
    }
    it->second = device;
    return true;
}

bool DeviceRegistry::addMeasurement(const MeasurementDevice& device)
{
    std::unique_lock lock(mutex_);
    return measurements_.emplace(device.id, device).second;
}

bool DeviceRegistry::addActuator(const ActuatorDevice& device)
{
    std::unique_lock lock(mutex_);
    return actuators_.emplace(device.id, device).second;
}

bool DeviceRegistry::removeMeasurement(std::string_view id)
{
    std::unique_lock lock(mutex_);
    return measurements_.erase(std::string(id)) > 0;
}

bool DeviceRegistry::removeActuator(std::string_view id)
{
    std::unique_lock lock(mutex_);
    return actuators_.erase(std::string(id)) > 0;
}

bool DeviceRegistry::parseLine(std::string_view line, std::string& currentSection, MeasurementDevice& measurementBuffer,
                               ActuatorDevice& actuatorBuffer, bool& measurementActive, bool& actuatorActive)
{
    auto trim = [](std::string_view value) {
        const auto start = value.find_first_not_of(" \t\r");
        if (start == std::string_view::npos) {
            return std::string_view{};
        }
        const auto end = value.find_last_not_of(" \t\r");
        return value.substr(start, end - start + 1);
    };

    line = trim(line);
    if (line.empty() || line.front() == '#') {
        return false;
    }

    if (line.front() == '[' && line.back() == ']') {
        return true;
    }

    const auto separator = line.find('=');
    if (separator == std::string_view::npos) {
        return false;
    }

    const auto key = trim(line.substr(0, separator));
    const auto value = trim(line.substr(separator + 1));

    if (currentSection.starts_with(measurementPrefix)) {
        measurementActive = true;
        if (key == "name") {
            measurementBuffer.name = std::string(value);
        } else if (key == "channel") {
            measurementBuffer.channel = channelFromString(value);
        } else if (key == "ip") {
            measurementBuffer.ipAddress = std::string(value);
        } else if (key == "port") {
            measurementBuffer.port = static_cast<std::uint16_t>(std::stoi(std::string(value)));
        } else if (key == "driver") {
            measurementBuffer.driver = std::string(value);
        } else if (key == "scale") {
            measurementBuffer.scale = std::stod(std::string(value));
        } else if (key == "offset") {
            measurementBuffer.offset = std::stod(std::string(value));
        } else if (key == "poll_interval_ms") {
            measurementBuffer.pollInterval = std::chrono::milliseconds(std::stoll(std::string(value)));
        }
    } else if (currentSection.starts_with(actuatorPrefix)) {
        actuatorActive = true;
        if (key == "name") {
            actuatorBuffer.name = std::string(value);
        } else if (key == "channel") {
            actuatorBuffer.channel = channelFromString(value);
        } else if (key == "ip") {
            actuatorBuffer.ipAddress = std::string(value);
        } else if (key == "port") {
            actuatorBuffer.port = static_cast<std::uint16_t>(std::stoi(std::string(value)));
        } else if (key == "driver") {
            actuatorBuffer.driver = std::string(value);
        } else if (key == "min") {
            actuatorBuffer.minimum = std::stod(std::string(value));
        } else if (key == "max") {
            actuatorBuffer.maximum = std::stod(std::string(value));
        } else if (key == "default") {
            actuatorBuffer.defaultValue = std::stod(std::string(value));
        }
    }

    return false;
}

void DeviceRegistry::commitBuffers(const std::string&, MeasurementDevice&, ActuatorDevice&, bool, bool)
{
    // Unused: commit is handled inline in load()
}

std::string DeviceRegistry::makeSectionName(std::string_view kind, std::string_view id)
{
    return std::string(kind) + ':' + std::string(id);
}

} // namespace devices
