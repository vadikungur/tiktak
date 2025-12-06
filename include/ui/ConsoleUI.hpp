#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <cstdint>

namespace devices {
class DeviceRegistry;
}

namespace services {
class AcquisitionService;
class ActuationService;
}

namespace algorithms {
class AlgorithmEngine;
}

namespace ui {

class ConsoleUI {
public:
    ConsoleUI(devices::DeviceRegistry& registry, services::AcquisitionService& acquisition,
              services::ActuationService& actuation, algorithms::AlgorithmEngine& engine);

    void run();

private:
    void printHelp() const;
    void handleCommand(const std::string& line);
    void commandList(const std::string& target) const;
    void commandShow(const std::string& kind, const std::string& id) const;
    void commandEdit(const std::string& kind, const std::string& id, const std::string& field, const std::string& value);
    void commandWrite(const std::string& id, const std::string& valueText);
    void commandSnapshot() const;
    void commandCreateAlgorithm(const std::string& name);

    static std::optional<double> parseDouble(std::string_view text);
    static std::optional<std::int64_t> parseInt64(std::string_view text);
    static std::optional<std::uint16_t> parsePort(std::string_view text);

    devices::DeviceRegistry& registry_;
    services::AcquisitionService& acquisition_;
    services::ActuationService& actuation_;
    algorithms::AlgorithmEngine& engine_;
};

} // namespace ui
