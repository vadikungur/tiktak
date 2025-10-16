#include "algorithms/AlgorithmEngine.hpp"
#include "devices/DeviceRegistry.hpp"
#include "network/EthernetTransport.hpp"
#include "services/AcquisitionService.hpp"
#include "services/ActuationService.hpp"
#include "ui/ConsoleUI.hpp"

#include <filesystem>
#include <iostream>

namespace {
std::filesystem::path resolveModulesPath(const std::filesystem::path& executableDir)
{
    std::filesystem::path candidates[] = {
        std::filesystem::current_path() / "modules",
        executableDir / "modules",
        executableDir / "../modules"
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return std::filesystem::current_path() / "modules";
}
} // namespace

int main(int argc, char** argv)
{
    std::filesystem::path config = "config/devices.cfg";
    if (argc > 1) {
        config = argv[1];
    }

    devices::DeviceRegistry registry(config);
    if (!registry.load()) {
        std::cerr << "Failed to load configuration from " << config << std::endl;
        return 1;
    }

    network::EthernetTransport transport;
    services::AcquisitionService acquisition(registry, transport);
    services::ActuationService actuation(registry, transport);
    algorithms::AlgorithmEngine engine(acquisition, actuation);

    acquisition.start();

    std::filesystem::path executable = std::filesystem::absolute(argv[0]).parent_path();
    auto modulesDir = resolveModulesPath(executable);
    if (!engine.loadModules(modulesDir)) {
        std::cerr << "Warning: no algorithm modules loaded from " << modulesDir << std::endl;
    }
    engine.start();

    ui::ConsoleUI console(registry, acquisition, actuation, engine);
    console.run();

    engine.stop();
    acquisition.stop();

    return 0;
}
