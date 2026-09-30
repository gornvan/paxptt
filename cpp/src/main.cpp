#include <QApplication>
#include <QDebug>
#include <QMetaObject>
#include <QProcess>
#include <QTimer>
#include <atomic>
#include <memory>

#include <X11/Xlib.h>

#include "config_manager.hpp"
#include "ptt_input_backend.hpp"
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

} // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("paxp2t");

    const AppConfig config = ConfigManager::readConfig();

    PulseAudioController pulse(config.cacheInputs);
    SoundController sound;
    TrayIconManager tray;

    std::atomic<bool> pttIsDown{false};

    auto applyMute = [&]() {
        pulse.muteAllRecordingSources();
        sound.playMute();
        tray.setIconState(false);
    };

    auto onPress = [&](int) {
        QMetaObject::invokeMethod(
            &app,
            [&]() {
                const bool wasDown = pttIsDown.exchange(true);
                if (wasDown) {
                    return;
                }
                sound.playUnmute();
                pulse.unmuteAllRecordingSources();
                tray.setIconState(true);
            },
            Qt::QueuedConnection);
    };

    auto onRelease = [&](int) {
        QMetaObject::invokeMethod(
            &app,
            [&]() {
                pttIsDown.store(false);
                QTimer::singleShot(config.muteDelayMs, &app, [&]() {
                    if (!pttIsDown.load()) {
                        applyMute();
                    }
                });
            },
            Qt::QueuedConnection);
    };

    sound.ensureAndLoadSounds();
    applyMute();

    if (config.showTrayIcon) {
        tray.showIcon(
            [&]() { openConfigFile(ConfigManager::configPath()); },
            [&]() { QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection); });
    }

    if (!qEnvironmentVariable("DISPLAY").isEmpty()) {
        XInitThreads();
    }

    std::unique_ptr<PttInputBackend> input = createPttInputBackend(config, onPress, onRelease);
    if (!input) {
        qCritical() << "No PTT input backend available.";
        qCritical() << "Prefer evdev: add your user to the 'input' group and set BIND_PTT in"
                    << ConfigManager::configPath();
        qCritical() << "On X11, XRecord fallback also requires DISPLAY and the XRecord extension.";
        return 1;
    }
    qInfo() << "PTT input:" << input->backendName();

    const int exitCode = app.exec();

    input->stop();
    tray.hideIcon();
    pulse.unmuteAllRecordingSources();
    return exitCode;
}
