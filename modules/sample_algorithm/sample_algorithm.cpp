#include "algorithms/AlgorithmModule.hpp"
#include "services/ActuationService.hpp"

#include <algorithm>
#include <iostream>

namespace {

class SampleAlgorithm final : public algorithms::AlgorithmModule {
public:
    std::string name() const override { return "sample_algorithm"; }
    std::chrono::milliseconds interval() const override { return std::chrono::milliseconds(1000); }

    void compute(const algorithms::DeviceSnapshot& snapshot, services::ActuationService& actuation) override
    {
        auto temperature = snapshot.find("temperature_sensor");
        if (temperature == snapshot.end()) {
            return;
        }

        const double desired = 42.0;
        const double current = temperature->second.value;
        const double error = desired - current;
        const double output = std::clamp(error * 0.1, -10.0, 10.0);

        if (!actuation.writeValue("heater", output, services::ActuationService::WriteSource::Algorithm)) {
            std::cerr << "SampleAlgorithm: failed to write to heater" << std::endl;
        }
    }
};

} // namespace

extern "C" algorithms::AlgorithmModule* create_module()
{
    return new SampleAlgorithm();
}

extern "C" void destroy_module(algorithms::AlgorithmModule* module)
{
    delete module;
}
