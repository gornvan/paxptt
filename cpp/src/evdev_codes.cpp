#include "evdev_codes.hpp"

#include "evdev_code_name_table.hpp"

#include <linux/input-event-codes.h>

#include <QString>

std::optional<int> evdevCodeFromToken(const QString &token) {
    QString t = token.trimmed();
    if (t.isEmpty() || t.compare(QStringLiteral("none"), Qt::CaseInsensitive) == 0) {
        return std::nullopt;
    }

    bool numericOk = false;
    const int asInt = t.toInt(&numericOk);
    if (numericOk && t == QString::number(asInt)) {
        return asInt;
    }

    t = t.toUpper();
    const auto it = evdevNameToCodeTable().constFind(t);
    if (it != evdevNameToCodeTable().constEnd()) {
        return *it;
    }
    return std::nullopt;
}

bool isEvdevButtonCode(int code) {
    return code >= BTN_MISC;
}

bool isEvdevKeyCode(int code) {
    return code >= 1 && code < BTN_MISC;
}
