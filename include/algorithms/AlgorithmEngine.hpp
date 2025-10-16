#pragma once

#include "algorithms/AlgorithmModule.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace services {
class AcquisitionService;
class ActuationService;
}

namespace algorithms {

class AlgorithmEngine {
public:
    AlgorithmEngine(services::AcquisitionService& acquisition, services::ActuationService& actuation);
    ~AlgorithmEngine();

    bool loadModules(const std::filesystem::path& directory);

    void start();
    void stop();

private:
    struct LoadedModule {
        ModulePtr instance;
        void* handle{nullptr};
        DestroyModuleFn destroy{nullptr};
        std::chrono::milliseconds interval{1000};
        std::chrono::steady_clock::time_point nextRun;
    };

    void run();

    services::AcquisitionService& acquisition_;
    services::ActuationService& actuation_;
    std::atomic<bool> running_{false};
    std::thread worker_;
    std::mutex modulesMutex_;
    std::vector<LoadedModule> modules_;
};

} // namespace algorithms
