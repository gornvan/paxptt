#pragma once

#include "ptt_input_backend.hpp"
#include "keyboard_binder.hpp"
#include "mouse_binder.hpp"

class XRecordPttInputBackend : public PttInputBackend {
public:
    XRecordPttInputBackend();
    ~XRecordPttInputBackend() override;

    bool loadBindingsFromConfig(const AppConfig &config, const Callback &onPress,
                                const Callback &onRelease) override;

    QString backendName() const override;

    bool start() override;

    void stop() override;

private:
    void bindFromEvdevCode(int evdevCode, const Callback &onPress, const Callback &onRelease);
    static int resolveKeycodeFromKeysym(const QString &keysymName);

    MouseBinder mouseBinder_;
    KeyboardBinder keyboardBinder_;
    bool anyBinding_{false};
};
