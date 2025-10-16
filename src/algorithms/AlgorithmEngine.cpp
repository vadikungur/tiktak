#include "algorithms/AlgorithmEngine.hpp"

#include "services/AcquisitionService.hpp"
#include "services/ActuationService.hpp"

#include <dlfcn.h>

#include <filesystem>
#include <iostream>

namespace algorithms {

AlgorithmEngine::AlgorithmEngine(services::AcquisitionService& acquisition, services::ActuationService& actuation)
    : acquisition_(acquisition)
    , actuation_(actuation)
{
}

AlgorithmEngine::~AlgorithmEngine()
{
    stop();
    std::scoped_lock lock(modulesMutex_);
    for (auto& module : modules_) {
        if (module.destroy) {
            module.destroy(module.instance.release());
        }
        if (module.handle) {
            ::dlclose(module.handle);
        }
    }
    modules_.clear();
}

bool AlgorithmEngine::loadModules(const std::filesystem::path& directory)
{
    if (!std::filesystem::exists(directory)) {
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".so") {
            continue;
        }

        void* handle = ::dlopen(entry.path().c_str(), RTLD_NOW);
        if (!handle) {
            std::cerr << "Failed to load module " << entry.path() << ": " << ::dlerror() << std::endl;
            continue;
        }

        auto create = reinterpret_cast<CreateModuleFn>(::dlsym(handle, "create_module"));
        auto destroy = reinterpret_cast<DestroyModuleFn>(::dlsym(handle, "destroy_module"));
        if (!create || !destroy) {
            std::cerr << "Invalid module interface in " << entry.path() << std::endl;
            ::dlclose(handle);
            continue;
        }

        ModulePtr instance(create());
        if (!instance) {
            std::cerr << "Module factory returned null for " << entry.path() << std::endl;
            ::dlclose(handle);
            continue;
        }

        LoadedModule module{};
        module.handle = handle;
        module.destroy = destroy;
        module.interval = instance->interval();
        module.nextRun = std::chrono::steady_clock::now() + module.interval;
        module.instance = std::move(instance);

        std::scoped_lock lock(modulesMutex_);
        modules_.push_back(std::move(module));
        std::cout << "Loaded algorithm module: " << modules_.back().instance->name() << std::endl;
    }

    return true;
}

void AlgorithmEngine::start()
{
    if (running_.exchange(true)) {
        return;
    }

    worker_ = std::thread([this]() { run(); });
}

void AlgorithmEngine::stop()
{
    if (!running_.exchange(false)) {
        return;
    }
    if (worker_.joinable()) {
        worker_.join();
    }
}

void AlgorithmEngine::run()
{
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        const auto now = std::chrono::steady_clock::now();
        algorithms::DeviceSnapshot snapshot = acquisition_.snapshot();

        std::scoped_lock lock(modulesMutex_);
        for (auto& module : modules_) {
            if (now >= module.nextRun) {
                module.instance->compute(snapshot, actuation_);
                module.nextRun = now + module.interval;
            }
        }
    }
}

} // namespace algorithms
