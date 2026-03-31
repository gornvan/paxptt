#pragma once

#include <QString>
#include <QVariantMap>

struct AppConfig {
    int bindMouseButton = 9;
    int bindKeyboardKey = 66;
    bool showTrayIcon = true;
    int muteDelayMs = 0;
    bool cacheInputs = true;
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
