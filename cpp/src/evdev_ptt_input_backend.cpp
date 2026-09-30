#include "evdev_ptt_input_backend.hpp"

#include "config_manager.hpp"
#include "evdev_codes.hpp"
#include "evdev_device_paths.hpp"

#include <QDebug>

#include <linux/input.h>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

EvdevPttInputBackend::EvdevPttInputBackend() = default;

EvdevPttInputBackend::~EvdevPttInputBackend() {
    stop();
}

bool EvdevPttInputBackend::loadBindingsFromConfig(const AppConfig &config, const Callback &onPress,
                                                  const Callback &onRelease) {
    bindings_.clear();
    for (const QString &token : config.bindPtt) {
        const std::optional<int> code = evdevCodeFromToken(token);
        if (!code) {
            qWarning() << "Invalid BIND_PTT token (evdev):" << token;
            continue;
        }
        bindEvdevCode(*code, onPress, onRelease);
    }
    return !bindings_.empty();
}

QString EvdevPttInputBackend::backendName() const {
    return QStringLiteral("evdev");
}

void EvdevPttInputBackend::bindEvdevCode(int code, const Callback &onPress, const Callback &onRelease) {
    bindings_[code] = Binding{onPress, onRelease};
}

bool EvdevPttInputBackend::openInputDevices() {
    closeInputDevices();
    const QStringList paths = discoverEvdevInputDevicePaths();
    qInfo() << "evdev: discovered" << paths.size() << "input device path(s)";
    for (const QString &path : paths) {
        const QByteArray pathUtf8 = path.toUtf8();
        const int fd = open(pathUtf8.constData(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            qInfo() << "evdev: skip" << path << "-" << std::strerror(errno);
            continue;
        }
        qInfo() << "evdev: opened" << path;
        deviceFds_.push_back(fd);
    }
    if (deviceFds_.empty()) {
        qWarning() << "evdev: no input devices could be opened (need 'input' group or root?)";
        return false;
    }
    return true;
}

void EvdevPttInputBackend::closeInputDevices() {
    for (int fd : deviceFds_) {
        close(fd);
    }
    deviceFds_.clear();
}

bool EvdevPttInputBackend::start() {
    if (started_) {
        return true;
    }
    if (bindings_.empty()) {
        return false;
    }
    if (!openInputDevices()) {
        return false;
    }
    running_.store(true);
    started_ = true;
    worker_ = std::thread(&EvdevPttInputBackend::runPollLoop, this);
    return true;
}

void EvdevPttInputBackend::stop() {
    if (!started_) {
        return;
    }
    running_.store(false);
    if (worker_.joinable()) {
        worker_.join();
    }
    closeInputDevices();
    started_ = false;
}

void EvdevPttInputBackend::handleEvKey(int code, int value) {
    if (value != 0 && value != 1) {
        return;
    }
    const auto it = bindings_.find(code);
    if (it == bindings_.end()) {
        return;
    }
    if (value == 1 && it->second.onPress) {
        it->second.onPress(code);
    } else if (value == 0 && it->second.onRelease) {
        it->second.onRelease(code);
    }
}

void EvdevPttInputBackend::runPollLoop() {
    std::vector<pollfd> pollfds;
    pollfds.reserve(deviceFds_.size());
    for (int fd : deviceFds_) {
        pollfds.push_back(pollfd{fd, POLLIN, 0});
    }

    input_event ev{};
    while (running_.load()) {
        const int ret = poll(pollfds.data(), static_cast<nfds_t>(pollfds.size()), 200);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (ret == 0) {
            continue;
        }

        for (const pollfd &pfd : pollfds) {
            if (!(pfd.revents & POLLIN)) {
                continue;
            }
            while (running_.load()) {
                const ssize_t n = read(pfd.fd, &ev, sizeof(ev));
                if (n < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }
                    break;
                }
                if (n != static_cast<ssize_t>(sizeof(ev))) {
                    break;
                }
                if (ev.type == EV_KEY) {
                    handleEvKey(ev.code, ev.value);
                }
            }
        }
    }
}
