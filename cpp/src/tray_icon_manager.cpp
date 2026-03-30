#include "tray_icon_manager.hpp"

#include <QAction>
#include <QColor>
#include <QDebug>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>

TrayIconManager::TrayIconManager(QObject *parent)
    : QObject(parent),
      activeIcon_(makeCircleIcon(QColor("#4CAF50"))),
      inactiveIcon_(makeCircleIcon(QColor("#6666aa"))) {
}

TrayIconManager::~TrayIconManager() {
    hideIcon();
}

void TrayIconManager::showIcon(const std::function<void()> &onOpenConfig,
                               const std::function<void()> &onTerminate) {
    if (trayIcon_) {
        return;
    }
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "System tray is not available on this desktop. Continuing without tray icon.";
        return;
    }

    trayIcon_ = new QSystemTrayIcon(this);
    menu_ = new QMenu();

    QAction *openConfigAction = menu_->addAction("Open config");
    QObject::connect(openConfigAction, &QAction::triggered, this, [onOpenConfig]() {
        if (onOpenConfig) {
            onOpenConfig();
        }
    });

    QAction *terminateAction = menu_->addAction("Terminate");
    QObject::connect(terminateAction, &QAction::triggered, this, [onTerminate]() {
        if (onTerminate) {
            onTerminate();
        }
    });

    trayIcon_->setIcon(inactiveIcon_);
    trayIcon_->setToolTip("PaX Push-to-Talk");
    trayIcon_->setContextMenu(menu_);
    trayIcon_->show();
}

void TrayIconManager::hideIcon() {
    if (!trayIcon_) {
        return;
    }
    trayIcon_->hide();
    trayIcon_->deleteLater();
    trayIcon_ = nullptr;
    if (menu_) {
        menu_->deleteLater();
        menu_ = nullptr;
    }
}

bool TrayIconManager::isAvailable() const {
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void TrayIconManager::setIconState(bool active) {
    if (!trayIcon_) {
        return;
    }
    trayIcon_->setIcon(active ? activeIcon_ : inactiveIcon_);
}

QIcon TrayIconManager::makeCircleIcon(const QColor &color) {
    constexpr int size = 64;
    constexpr int margin = 4;

    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(margin, margin, size - margin * 2, size - margin * 2);
    painter.end();

    return QIcon(pixmap);
}
