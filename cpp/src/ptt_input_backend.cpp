#include "config_manager.hpp"

#include <QDebug>

#include "evdev_ptt_input_backend.hpp"
#include "ptt_input_backend.hpp"
#include "xrecord_ptt_input_backend.hpp"

namespace {

std::unique_ptr<PttInputBackend> tryBackend(std::unique_ptr<PttInputBackend> backend, const AppConfig &config,
                                            const PttInputBackend::Callback &onPress,
                                            const PttInputBackend::Callback &onRelease) {
    if (!backend->loadBindingsFromConfig(config, onPress, onRelease)) {
        qInfo() << "PTT backend" << backend->backendName() << "has no usable bindings from config";
        return nullptr;
    }
    if (!backend->start()) {
        qInfo() << "PTT backend" << backend->backendName() << "failed to start";
        backend->stop();
        return nullptr;
    }
    return backend;
}

} // namespace

std::unique_ptr<PttInputBackend> createPttInputBackend(const AppConfig &config,
                                                       const PttInputBackend::Callback &onPress,
                                                       const PttInputBackend::Callback &onRelease) {
    if (auto evdev = tryBackend(std::make_unique<EvdevPttInputBackend>(), config, onPress, onRelease)) {
        return evdev;
    }

    qInfo() << "Trying XRecord PTT fallback (requires X11 session and DISPLAY)";

    if (qEnvironmentVariable("DISPLAY").isEmpty()) {
        qWarning() << "DISPLAY is not set; XRecord fallback unavailable";
        return nullptr;
    }

    if (auto xrec = tryBackend(std::make_unique<XRecordPttInputBackend>(), config, onPress, onRelease)) {
        return xrec;
    }

    return nullptr;
}
