#include "config_manager.hpp"

#include <QDebug>

#include "evdev_codes.hpp"
#include "evdev_to_x11_map.hpp"
#include "xrecord_ptt_input_backend.hpp"

#include <X11/Xlib.h>
#include <X11/keysym.h>

XRecordPttInputBackend::XRecordPttInputBackend() = default;

XRecordPttInputBackend::~XRecordPttInputBackend() {
    stop();
}

bool XRecordPttInputBackend::loadBindingsFromConfig(const AppConfig &config, const Callback &onPress,
                                                    const Callback &onRelease) {
    anyBinding_ = false;
    for (const QString &token : config.bindPtt) {
        const std::optional<int> code = evdevCodeFromToken(token);
        if (!code) {
            qWarning() << "Invalid BIND_PTT token (XRecord):" << token;
            continue;
        }
        bindFromEvdevCode(*code, onPress, onRelease);
    }
    return anyBinding_;
}

QString XRecordPttInputBackend::backendName() const {
    return QStringLiteral("xrecord");
}

bool XRecordPttInputBackend::start() {
    return anyBinding_;
}

void XRecordPttInputBackend::stop() {
    mouseBinder_.stop();
    keyboardBinder_.stop();
}

void XRecordPttInputBackend::bindFromEvdevCode(int evdevCode, const Callback &onPress,
                                               const Callback &onRelease) {
    if (isEvdevButtonCode(evdevCode)) {
        const std::optional<int> xButton = evdevCodeToX11MouseButton(evdevCode);
        if (!xButton) {
            qWarning() << "No X11 mapping for evdev button code" << evdevCode;
            return;
        }
        mouseBinder_.bind(*xButton, onPress, onRelease);
        anyBinding_ = true;
        return;
    }

    if (isEvdevKeyCode(evdevCode)) {
        const std::optional<QString> keysymName = evdevCodeToXKeysymName(evdevCode);
        if (!keysymName) {
            qWarning() << "No X11 keysym mapping for evdev key code" << evdevCode;
            return;
        }
        const int keycode = resolveKeycodeFromKeysym(*keysymName);
        if (keycode <= 0) {
            qWarning() << "X11 keycode unavailable for keysym" << *keysymName << "(evdev code" << evdevCode << ")";
            return;
        }
        keyboardBinder_.bind(keycode, onPress, onRelease);
        anyBinding_ = true;
        return;
    }

    qWarning() << "Unknown evdev code for XRecord binding:" << evdevCode;
}

int XRecordPttInputBackend::resolveKeycodeFromKeysym(const QString &keysymName) {
    const QString trimmed = keysymName.trimmed();
    if (trimmed.isEmpty()) {
        return 0;
    }

    const QByteArray keysymUtf8 = trimmed.toUtf8();
    const KeySym keysym = XStringToKeysym(keysymUtf8.constData());
    if (keysym == NoSymbol) {
        qWarning() << "Invalid X11 keysym:" << keysymName;
        return -1;
    }

    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        qWarning() << "Unable to open X display while resolving keysym:" << keysymName;
        return -1;
    }

    const KeyCode keycode = XKeysymToKeycode(display, keysym);
    XCloseDisplay(display);
    if (keycode == 0) {
        qWarning() << "X11 keysym is not mapped to any keycode on this layout:" << keysymName;
        return -1;
    }

    return static_cast<int>(keycode);
}
