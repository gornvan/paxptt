#pragma once

#include "ptt_input_backend.hpp"

#include <atomic>
#include <thread>
#include <unordered_map>
#include <vector>

class EvdevPttInputBackend : public PttInputBackend {
public:
    EvdevPttInputBackend();
    ~EvdevPttInputBackend() override;

    bool loadBindingsFromConfig(const AppConfig &config, const Callback &onPress,
                                const Callback &onRelease) override;

    QString backendName() const override;

    bool start() override;

    void stop() override;

private:
    struct Binding {
        Callback onPress;
        Callback onRelease;
    };

    void bindEvdevCode(int code, const Callback &onPress, const Callback &onRelease);
    bool openInputDevices();
    void closeInputDevices();
    void runPollLoop();
    void handleEvKey(int code, int value);

    std::unordered_map<int, Binding> bindings_;
    std::vector<int> deviceFds_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    bool started_{false};
};
