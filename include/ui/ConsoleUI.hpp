#pragma once

#include <functional>
#include <string>

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
    void commandWrite(const std::string& id, double value);
    void commandSnapshot() const;
    void commandCreateAlgorithm(const std::string& name);

    devices::DeviceRegistry& registry_;
    services::AcquisitionService& acquisition_;
    services::ActuationService& actuation_;
    algorithms::AlgorithmEngine& engine_;
};

} // namespace ui
