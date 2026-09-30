#include "evdev_device_paths.hpp"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

namespace {

QStringList globById(const char *suffix) {
    QDir dir(QStringLiteral("/dev/input/by-id"));
    if (!dir.exists()) {
        return {};
    }
    const QString pattern = QStringLiteral("*%1").arg(QString::fromLatin1(suffix));
    // Do not use QDir::Readable — it can omit nodes that open() still succeeds on (match Python glob).
    const QStringList entries = dir.entryList({pattern}, QDir::System, QDir::Name);
    QStringList paths;
    for (const QString &name : entries) {
        paths.append(dir.absoluteFilePath(name));
    }
    return paths;
}

QStringList handlersFromBlock(const QString &block) {
    static const QRegularExpression handlerRe(QStringLiteral(R"(H: Handlers=(.+))"));
    const QRegularExpressionMatch match = handlerRe.match(block);
    if (!match.hasMatch()) {
        return {};
    }
    QStringList out;
    const QStringList handlers = match.captured(1).split(QChar(' '), Qt::SkipEmptyParts);
    for (const QString &h : handlers) {
        if (h.startsWith(QStringLiteral("event"))) {
            out.append(QStringLiteral("/dev/input/%1").arg(h));
        }
    }
    return out;
}

QStringList discoverFromProc(bool wantMouse, bool wantKbd) {
    QFile file(QStringLiteral("/proc/bus/input/devices"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    const QString content = QString::fromUtf8(file.readAll());
    QStringList out;
    for (const QString &block : content.split(QStringLiteral("\n\n"), Qt::SkipEmptyParts)) {
        const QString lower = block.toLower();
        const bool isMouse = lower.contains(QStringLiteral("mouse"));
        const bool isKbd = lower.contains(QStringLiteral("keyboard"));
        if ((wantMouse && isMouse) || (wantKbd && isKbd)) {
            out.append(handlersFromBlock(block));
        }
    }
    return out;
}

} // namespace

QStringList discoverEvdevInputDevicePaths() {
    QSet<QString> paths;
    for (const QString &p : globById("-event-mouse")) {
        paths.insert(p);
    }
    for (const QString &p : globById("-event-kbd")) {
        paths.insert(p);
    }
    // Always merge /proc discovery: by-id may list keyboards but miss mice (or vice versa).
    for (const QString &p : discoverFromProc(true, false)) {
        paths.insert(p);
    }
    for (const QString &p : discoverFromProc(false, true)) {
        paths.insert(p);
    }

    QStringList sorted = paths.values();
    sorted.sort();
    return sorted;
}
