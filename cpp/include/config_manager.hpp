#pragma once

#include <QList>
#include <QString>
#include <QVariantMap>

struct AppConfig {
    QList<QString> bindPtt = {QStringLiteral("BTN_EXTRA"), QStringLiteral("KEY_CAPSLOCK")};
    bool cacheInputs = true;
    int muteDelayMs = 0;
    bool showTrayIcon = true;
};

class ConfigManager {
public:
    static QString configDirPath();
    static QString configPath();
    static AppConfig readConfig();

private:
    static bool parseBool(const QString &value, bool &out);
    static bool parseInt(const QString &value, int &out);
    static bool writeConfig(const AppConfig &config, const QVariantMap &extraKeys);
};
