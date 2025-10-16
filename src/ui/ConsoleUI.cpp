#include "ui/ConsoleUI.hpp"

#include "algorithms/AlgorithmEngine.hpp"
#include "devices/DeviceRegistry.hpp"
#include "services/AcquisitionService.hpp"
#include "services/ActuationService.hpp"

#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace ui {

namespace {
std::vector<std::string> split(const std::string& line)
{
    std::istringstream stream(line);
    std::vector<std::string> parts;
    std::string token;
    while (stream >> token) {
        parts.push_back(token);
    }
    return parts;
}

std::string toLower(std::string value)
{
    for (auto& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

void printDevice(const devices::MeasurementDevice& device)
{
    std::cout << "Sensor " << device.id << " (" << device.name << ")\n"
              << "  channel: " << devices::channelToString(device.channel) << '\n'
              << "  address: " << device.ipAddress << ':' << device.port << '\n'
              << "  driver: " << device.driver << '\n'
              << "  scale: " << device.scale << " offset: " << device.offset << '\n'
              << "  poll: " << device.pollInterval.count() << " ms" << std::endl;
}

void printDevice(const devices::ActuatorDevice& device)
{
    std::cout << "Actuator " << device.id << " (" << device.name << ")\n"
              << "  channel: " << devices::channelToString(device.channel) << '\n'
              << "  address: " << device.ipAddress << ':' << device.port << '\n'
              << "  driver: " << device.driver << '\n'
              << "  range: [" << device.minimum << ", " << device.maximum << "]\n"
              << "  default: " << device.defaultValue << std::endl;
}

} // namespace

ConsoleUI::ConsoleUI(devices::DeviceRegistry& registry, services::AcquisitionService& acquisition,
                     services::ActuationService& actuation, algorithms::AlgorithmEngine& engine)
    : registry_(registry)
    , acquisition_(acquisition)
    , actuation_(actuation)
    , engine_(engine)
{
}

void ConsoleUI::run()
{
    std::cout << "Industrial Controller Console" << std::endl;
    printHelp();

    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        if (line == "exit" || line == "quit") {
            break;
        }
        handleCommand(line);
    }
}

void ConsoleUI::printHelp() const
{
    std::cout << "Commands:\n"
              << "  help                       - show this message\n"
              << "  list sensors|actuators     - list devices\n"
              << "  show sensor|actuator <id>  - display device configuration\n"
              << "  edit sensor|actuator <id> <field> <value> - update configuration field\n"
              << "  write <actuator_id> <value> - write manual value to actuator\n"
              << "  snapshot                   - print latest measurement snapshot\n"
              << "  create algorithm <name>    - scaffold a new algorithm module\n"
              << "  quit/exit                  - terminate console" << std::endl;
}

void ConsoleUI::handleCommand(const std::string& line)
{
    auto tokens = split(line);
    if (tokens.empty()) {
        return;
    }

    const auto command = toLower(tokens[0]);
    if (command == "help") {
        printHelp();
    } else if (command == "list" && tokens.size() >= 2) {
        commandList(toLower(tokens[1]));
    } else if (command == "show" && tokens.size() >= 3) {
        commandShow(toLower(tokens[1]), tokens[2]);
    } else if (command == "edit" && tokens.size() >= 5) {
        commandEdit(toLower(tokens[1]), tokens[2], toLower(tokens[3]), tokens[4]);
    } else if (command == "write" && tokens.size() >= 3) {
        commandWrite(tokens[1], std::stod(tokens[2]));
    } else if (command == "snapshot") {
        commandSnapshot();
    } else if (command == "create" && tokens.size() >= 3 && toLower(tokens[1]) == "algorithm") {
        commandCreateAlgorithm(tokens[2]);
    } else {
        std::cout << "Unknown command. Type 'help' for usage." << std::endl;
    }
}

void ConsoleUI::commandList(const std::string& target) const
{
    if (target == "sensors") {
        for (const auto& device : registry_.measurementDevices()) {
            printDevice(device);
        }
    } else if (target == "actuators") {
        for (const auto& device : registry_.actuatorDevices()) {
            printDevice(device);
        }
    } else {
        std::cout << "Unknown list target: " << target << std::endl;
    }
}

void ConsoleUI::commandShow(const std::string& kind, const std::string& id) const
{
    if (kind == "sensor") {
        if (auto device = registry_.findMeasurement(id)) {
            printDevice(*device);
        } else {
            std::cout << "Sensor not found: " << id << std::endl;
        }
    } else if (kind == "actuator") {
        if (auto device = registry_.findActuator(id)) {
            printDevice(*device);
        } else {
            std::cout << "Actuator not found: " << id << std::endl;
        }
    }
}

void ConsoleUI::commandEdit(const std::string& kind, const std::string& id, const std::string& field, const std::string& value)
{
    if (kind == "sensor") {
        auto device = registry_.findMeasurement(id);
        if (!device) {
            std::cout << "Sensor not found: " << id << std::endl;
            return;
        }
        if (field == "name") {
            device->name = value;
        } else if (field == "channel") {
            device->channel = devices::channelFromString(value);
        } else if (field == "ip") {
            device->ipAddress = value;
        } else if (field == "port") {
            device->port = static_cast<std::uint16_t>(std::stoi(value));
        } else if (field == "driver") {
            device->driver = value;
        } else if (field == "scale") {
            device->scale = std::stod(value);
        } else if (field == "offset") {
            device->offset = std::stod(value);
        } else if (field == "poll") {
            device->pollInterval = std::chrono::milliseconds(std::stoll(value));
        } else {
            std::cout << "Unsupported field for sensor." << std::endl;
            return;
        }
        if (registry_.updateMeasurement(*device)) {
            registry_.save();
            std::cout << "Sensor updated." << std::endl;
        }
    } else if (kind == "actuator") {
        auto device = registry_.findActuator(id);
        if (!device) {
            std::cout << "Actuator not found: " << id << std::endl;
            return;
        }
        if (field == "name") {
            device->name = value;
        } else if (field == "channel") {
            device->channel = devices::channelFromString(value);
        } else if (field == "ip") {
            device->ipAddress = value;
        } else if (field == "port") {
            device->port = static_cast<std::uint16_t>(std::stoi(value));
        } else if (field == "driver") {
            device->driver = value;
        } else if (field == "min") {
            device->minimum = std::stod(value);
        } else if (field == "max") {
            device->maximum = std::stod(value);
        } else if (field == "default") {
            device->defaultValue = std::stod(value);
        } else {
            std::cout << "Unsupported field for actuator." << std::endl;
            return;
        }
        if (registry_.updateActuator(*device)) {
            registry_.save();
            std::cout << "Actuator updated." << std::endl;
        }
    }
}

void ConsoleUI::commandWrite(const std::string& id, double value)
{
    if (actuation_.writeValue(id, value, services::ActuationService::WriteSource::Manual)) {
        std::cout << "Value sent to actuator." << std::endl;
    } else {
        std::cout << "Failed to send value." << std::endl;
    }
}

void ConsoleUI::commandSnapshot() const
{
    auto snapshot = acquisition_.snapshot();
    if (snapshot.empty()) {
        std::cout << "No measurements available." << std::endl;
        return;
    }

    for (const auto& [id, state] : snapshot) {
        const auto time = std::chrono::system_clock::to_time_t(state.timestamp);
        std::cout << id << ": " << state.value << " @ " << std::put_time(std::localtime(&time), "%F %T") << std::endl;
    }
}

void ConsoleUI::commandCreateAlgorithm(const std::string& name)
{
    std::filesystem::path dir = std::filesystem::path("modules") / name;
    std::filesystem::create_directories(dir);
    std::filesystem::path file = dir / (name + ".cpp");
    if (std::filesystem::exists(file)) {
        std::cout << "Algorithm source already exists: " << file << std::endl;
        return;
    }

    std::ofstream output(file);
    output << "#include \"algorithms/AlgorithmModule.hpp\"\n"
              "#include \"services/ActuationService.hpp\"\n"
              "\n"
              "namespace {\n"
              "class UserAlgorithm : public algorithms::AlgorithmModule {\n"
              "public:\n"
              "    std::string name() const override { return \"" << name << "\"; }\n"
              "    std::chrono::milliseconds interval() const override { return std::chrono::milliseconds(500); }\n"
              "    void compute(const algorithms::DeviceSnapshot& snapshot, services::ActuationService& actuation) override {\n"
              "        // TODO: implement control logic\n"
              "        (void)snapshot;\n"
              "        (void)actuation;\n"
              "    }\n"
              "};\n"
              "} // namespace\n"
              "\n"
              "extern \"C\" algorithms::AlgorithmModule* create_module() { return new UserAlgorithm(); }\n"
              "extern \"C\" void destroy_module(algorithms::AlgorithmModule* module) { delete module; }\n";

    std::cout << "Algorithm template created at " << file << std::endl;
}

} // namespace ui
