#pragma once

#include <QString>
#include <functional>
#include <memory>

struct AppConfig;

class PttInputBackend {
public:
    using Callback = std::function<void(int id)>;

    virtual ~PttInputBackend() = default;

    PttInputBackend(const PttInputBackend &) = delete;
    PttInputBackend &operator=(const PttInputBackend &) = delete;

    virtual bool loadBindingsFromConfig(const AppConfig &config, const Callback &onPress,
                                        const Callback &onRelease) = 0;

    virtual QString backendName() const = 0;

    virtual bool start() = 0;

    virtual void stop() = 0;

protected:
    PttInputBackend() = default;
};

std::unique_ptr<PttInputBackend> createPttInputBackend(const AppConfig &config,
                                                       const PttInputBackend::Callback &onPress,
                                                       const PttInputBackend::Callback &onRelease);
