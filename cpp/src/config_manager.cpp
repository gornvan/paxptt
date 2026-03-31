#include "config_manager.hpp"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace {

    QString trim(const QString &value) {
        return value.trimmed();
    }

    QString toYamlBool(bool value) {
        return value ? "true" : "false";
    }

}

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
        QString value = trim(line.mid(colon + 1));
        if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2) {
            value = value.mid(1, value.size() - 2);
        } else if (value.startsWith('\'') && value.endsWith('\'') && value.size() >= 2) {
            value = value.mid(1, value.size() - 2);
        }

        if (key == "BIND_MOUSE_BUTTON") {
            int out = defaults.bindMouseButton;
            if (parseInt(value, out)) {
                parsed.bindMouseButton = out;
            } else {
                needsRewrite = true;
            }
            continue;
        }
        if (key == "BIND_KEYBOARD_KEY") {
            int out = defaults.bindKeyboardKey;
            if (parseInt(value, out)) {
                parsed.bindKeyboardKey = out;
            } else {
                needsRewrite = true;
            }
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

    const bool missingKnown =
        !extras.contains("BIND_MOUSE_BUTTON") &&
        !extras.contains("BIND_KEYBOARD_KEY") &&
        !extras.contains("SHOW_TRAY_ICON") &&
        !extras.contains("MUTE_DELAY_MS");

    Q_UNUSED(missingKnown);

    // Rewrite if the file had invalid lines/values or is missing expected keys.
    if (needsRewrite) {
        writeConfig(parsed, extras);
        return parsed;
    }

    // Backfill missing keys by comparing against parsed defaults.
    QFile verifyFile(configPath());
    if (verifyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString content = QString::fromUtf8(verifyFile.readAll());
        if (!content.contains("BIND_MOUSE_BUTTON:") ||
            !content.contains("BIND_KEYBOARD_KEY:") ||
            !content.contains("SHOW_TRAY_ICON:") ||
            !content.contains("MUTE_DELAY_MS:") ||
            !content.contains("CACHE_INPUTS:")) {
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
    out << "BIND_MOUSE_BUTTON: " << config.bindMouseButton << "\n";
    out << "BIND_KEYBOARD_KEY: " << config.bindKeyboardKey << "\n";
    out << "SHOW_TRAY_ICON: " << toYamlBool(config.showTrayIcon) << "\n";
    out << "MUTE_DELAY_MS: " << qMax(0, config.muteDelayMs) << "\n";
    out << "CACHE_INPUTS: " << toYamlBool(config.cacheInputs) << "\n";

    for (auto it = extraKeys.constBegin(); it != extraKeys.constEnd(); ++it) {
        out << it.key() << ": " << it.value().toString() << "\n";
    }
    return true;
}
