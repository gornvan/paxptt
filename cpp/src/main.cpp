#include <QApplication>
#include <QDebug>
#include <QMetaObject>
#include <QProcess>
#include <QTimer>
#include <atomic>
#include <X11/Xlib.h>

#include "config_manager.hpp"
#include "keyboard_binder.hpp"
#include "mouse_binder.hpp"
#include "pulseaudio_controller.hpp"
#include "sound_controller.hpp"
#include "tray_icon_manager.hpp"

namespace {

void openConfigFile(const QString &path) {
    const QList<QStringList> candidates = {
        {"xdg-open", path},
        {"gio", "open", path},
        {"flatpak-spawn", "--host", "xdg-open", path},
    };

    for (const QStringList &cmd : candidates) {
        if (cmd.isEmpty()) {
            continue;
        }
        const QString program = cmd.first();
        const QStringList args = cmd.mid(1);
        if (QProcess::startDetached(program, args)) {
            return;
        }
    }
    qCritical() << "Failed to open config file with any of the following commands:";
    qCritical() << "`xdg-open`, `gio open`, `flatpak spawn --host xdg-open`";
    qCritical() << "Please check if any of the commands is available in your PATH.";
    qCritical() << "You can manually open the config file at " << path;
}

int resolveKeycodeFromKeysym(const QString &keysymName) {
    const QString trimmed = keysymName.trimmed();
    if (trimmed.isEmpty() || trimmed.compare("none", Qt::CaseInsensitive) == 0) {
        return 0;
    }

    const QByteArray keysymUtf8 = trimmed.toUtf8();
    const KeySym keysym = XStringToKeysym(keysymUtf8.constData());
    if (keysym == NoSymbol) {
        qWarning() << "Invalid X11 keysym in config:" << keysymName;
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

} // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("paxp2t");

    const QString sessionType = qEnvironmentVariable("XDG_SESSION_TYPE").toLower();
    if (sessionType != "x11") {
        qCritical() << "paxp2t supports X11 sessions only.";
        return 1;
    }
    if (qEnvironmentVariable("DISPLAY").isEmpty()) {
        qCritical() << "DISPLAY is not set. Unable to connect to X11.";
        return 1;
    }

    XInitThreads();

    const AppConfig config = ConfigManager::readConfig();

    PulseAudioController pulse(config.cacheInputs);
    SoundController sound;
    TrayIconManager tray;
    MouseBinder mouseBinder;
    KeyboardBinder keyboardBinder;

    std::atomic<bool> pttIsDown{false};

    auto applyMute = [&]() {
        pulse.muteAllRecordingSources();
        sound.playMute();
        QMetaObject::invokeMethod(&app, [&tray]() { tray.setIconState(false); }, Qt::QueuedConnection);
    };

    auto onPress = [&](int) {
        const bool wasDown = pttIsDown.exchange(true);
        if (wasDown) {
            return;
        }
        sound.playUnmute();
        pulse.unmuteAllRecordingSources();
        QMetaObject::invokeMethod(&app, [&tray]() { tray.setIconState(true); }, Qt::QueuedConnection);
    };

    auto onRelease = [&](int) {
        pttIsDown.store(false);
        QTimer::singleShot(config.muteDelayMs, &app, [&]() {
            if (!pttIsDown.load()) {
                applyMute();
            }
        });
    };

    sound.ensureSounds();
    applyMute();

    if (config.showTrayIcon) {
        tray.showIcon(
            [&]() { openConfigFile(ConfigManager::configPath()); },
            [&]() { QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection); });
    }

    if (config.bindMouseButton > 0) {
        mouseBinder.bind(config.bindMouseButton, onPress, onRelease);
    }
    const int keyboardKeycode = resolveKeycodeFromKeysym(config.bindKeyboardKeysym);
    if (keyboardKeycode > 0) {
        keyboardBinder.bind(keyboardKeycode, onPress, onRelease);
    }

    const int exitCode = app.exec();

    mouseBinder.stop();
    keyboardBinder.stop();
    tray.hideIcon();
    pulse.unmuteAllRecordingSources();
    return exitCode;
}
