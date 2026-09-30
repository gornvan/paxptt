#include "config_manager.hpp"

#include <QDir>
#include <QFile>
#include <QTextStream>

namespace {

QString trim(const QString &value) {
    return value.trimmed();
}

QString unquote(QString value) {
    value = trim(value);
    if (value.size() >= 2 &&
        ((value.startsWith('"') && value.endsWith('"')) || (value.startsWith('\'') && value.endsWith('\'')))) {
        return value.mid(1, value.size() - 2);
    }
    return value;
}

QStringList splitListItems(QString inner) {
    inner = trim(inner);
    if (inner.isEmpty()) {
        return {};
    }
    QStringList out;
    for (QString part : inner.split(',', Qt::SkipEmptyParts)) {
        part = unquote(trim(part));
        if (!part.isEmpty()) {
            out.append(part);
        }
    }
    return out;
}

QStringList parseStringListValue(const QString &raw) {
    QString value = trim(raw);
    if (value.startsWith('[') && value.endsWith(']')) {
        return splitListItems(value.mid(1, value.size() - 2));
    }
    if (value.isEmpty()) {
        return {};
    }
    return {unquote(value)};
}

QString formatYamlStringList(const QList<QString> &items) {
    if (items.isEmpty()) {
        return QStringLiteral("[]");
    }
    QStringList parts;
    for (const QString &item : items) {
        parts.append(item);
    }
    return QStringLiteral("[") + parts.join(QStringLiteral(", ")) + QStringLiteral("]");
}

QString toYamlBool(bool value) {
    return value ? "true" : "false";
}

QList<QString> normalizedPttTokens(const QList<QString> &raw) {
    QList<QString> out;
    for (QString token : raw) {
        token = trim(token);
        if (token.isEmpty() || token.compare(QStringLiteral("none"), Qt::CaseInsensitive) == 0) {
            continue;
        }
        out.append(token);
    }
    return out;
}

} // namespace

QString ConfigManager::configDirPath() {
    return QDir::homePath() + "/.local/paxp2t";
}

QString ConfigManager::configPath() {
    return configDirPath() + "/config.yml";
}

AppConfig ConfigManager::readConfig() {
    AppConfig defaults;
    AppConfig parsed = defaults;
    QVariantMap extras;
    bool needsRewrite = false;
    bool sawBindPtt = false;

    QFile file(configPath());
    if (!file.exists()) {
        writeConfig(defaults, {});
        return defaults;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return defaults;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString rawLine = in.readLine();
        const QString line = trim(rawLine);
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        const int colon = line.indexOf(':');
        if (colon <= 0) {
            needsRewrite = true;
            continue;
        }

        const QString key = trim(line.left(colon));
        const QString value = trim(line.mid(colon + 1));

        if (key == "BIND_PTT") {
            parsed.bindPtt = normalizedPttTokens(parseStringListValue(value));
            sawBindPtt = true;
            continue;
        }
        if (key == "BIND_MOUSE_BUTTON" || key == "BIND_MOUSE_BUTTONS" || key == "BIND_KEYBOARD_KEYSYM" ||
            key == "BIND_KEYBOARD_KEYSYMS" || key == "BIND_PTT_INPUT" || key == "BIND_INPUT") {
            needsRewrite = true;
            continue;
        }
        if (key == "SHOW_TRAY_ICON") {
            bool out = defaults.showTrayIcon;
            if (parseBool(value, out)) {
                parsed.showTrayIcon = out;
            } else {
                needsRewrite = true;
            }
            continue;
        }
        if (key == "MUTE_DELAY_MS") {
            int out = defaults.muteDelayMs;
            if (parseInt(value, out)) {
                parsed.muteDelayMs = qMax(0, out);
            } else {
                needsRewrite = true;
            }
            continue;
        }
        if (key == "CACHE_INPUTS") {
            bool out = defaults.cacheInputs;
            if (parseBool(value, out)) {
                parsed.cacheInputs = out;
            } else {
                needsRewrite = true;
            }
            continue;
        }

        extras.insert(key, value);
    }

    if (!sawBindPtt) {
        parsed.bindPtt = defaults.bindPtt;
        needsRewrite = true;
    }

    if (needsRewrite) {
        writeConfig(parsed, extras);
        return parsed;
    }

    QFile verifyFile(configPath());
    if (verifyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString content = QString::fromUtf8(verifyFile.readAll());
        if (!content.contains("BIND_PTT:") || !content.contains("SHOW_TRAY_ICON:") ||
            !content.contains("MUTE_DELAY_MS:") || !content.contains("CACHE_INPUTS:")) {
            writeConfig(parsed, extras);
        }
    }

    return parsed;
}

bool ConfigManager::parseBool(const QString &value, bool &out) {
    const QString v = value.trimmed().toLower();
    if (v == "1" || v == "true" || v == "yes" || v == "on" || v == "ON" || v == "TRUE" || v == "YES") {
        out = true;
        return true;
    }
    if (v == "0" || v == "false" || v == "no" || v == "off" || v == "OFF" || v == "FALSE" || v == "NO") {
        out = false;
        return true;
    }
    return false;
}

bool ConfigManager::parseInt(const QString &value, int &out) {
    bool ok = false;
    const int parsed = value.toInt(&ok);
    if (ok) {
        out = parsed;
        return true;
    }
    return false;
}

bool ConfigManager::writeConfig(const AppConfig &config, const QVariantMap &extraKeys) {
    QDir dir(configDirPath());
    if (!dir.exists() && !dir.mkpath(".")) {
        return false;
    }

    QFile file(configPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    QTextStream out(&file);
    out << "BIND_PTT: " << formatYamlStringList(config.bindPtt) << "\n";
    out << "CACHE_INPUTS: " << toYamlBool(config.cacheInputs) << "\n";
    out << "MUTE_DELAY_MS: " << qMax(0, config.muteDelayMs) << "\n";
    out << "SHOW_TRAY_ICON: " << toYamlBool(config.showTrayIcon) << "\n";

    for (auto it = extraKeys.constBegin(); it != extraKeys.constEnd(); ++it) {
        out << it.key() << ": " << it.value().toString() << "\n";
    }
    return true;
}
