#pragma once

#include "devices/DeviceConfig.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

namespace services {
class ActuationService;
}

namespace algorithms {

using DeviceSnapshot = std::unordered_map<std::string, devices::DeviceState>;

class AlgorithmModule {
public:
    virtual ~AlgorithmModule() = default;

    virtual std::string name() const = 0;
    virtual std::chrono::milliseconds interval() const = 0;
    virtual void compute(const DeviceSnapshot& snapshot, services::ActuationService& actuation) = 0;
};

using ModulePtr = std::unique_ptr<AlgorithmModule>;

using CreateModuleFn = AlgorithmModule* (*)();
using DestroyModuleFn = void (*)(AlgorithmModule*);

} // namespace algorithms
