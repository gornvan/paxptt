#pragma once

#include <QIcon>
#include <QObject>
#include <functional>

class QMenu;
class QSystemTrayIcon;

class TrayIconManager final : public QObject {
public:
    explicit TrayIconManager(QObject *parent = nullptr);
    ~TrayIconManager() override;

    void showIcon(const std::function<void()> &onOpenConfig,
                  const std::function<void()> &onTerminate);
    void hideIcon();
    bool isAvailable() const;
    void setIconState(bool active);

private:
    static QIcon makeCircleIcon(const QColor &color);

    QIcon activeIcon_;
    QIcon inactiveIcon_;
    QSystemTrayIcon *trayIcon_ = nullptr;
    QMenu *menu_ = nullptr;
};
