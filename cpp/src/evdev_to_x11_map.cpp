#include "evdev_to_x11_map.hpp"

#include "evdev_codes.hpp"

#include <linux/input-event-codes.h>

#include <QHash>

namespace {

QHash<int, int> buildEvdevToXButton() {
    QHash<int, int> map;
    map.insert(BTN_LEFT, 1);
    map.insert(BTN_MIDDLE, 2);
    map.insert(BTN_RIGHT, 3);
    map.insert(BTN_SIDE, 8);
    map.insert(BTN_EXTRA, 9);
    map.insert(BTN_FORWARD, 9);
    map.insert(BTN_BACK, 8);
    map.insert(BTN_TASK, 10);
    return map;
}

QHash<int, QString> buildEvdevToKeysym() {
    QHash<int, QString> map;
    const auto add = [&](int code, const char *keysym) { map.insert(code, QString::fromLatin1(keysym)); };

    add(KEY_CAPSLOCK, "Caps_Lock");
    add(KEY_SCROLLLOCK, "Scroll_Lock");
    add(KEY_SPACE, "space");
    add(KEY_ENTER, "Return");
    add(KEY_ESC, "Escape");
    add(KEY_PAUSE, "Pause");
    add(KEY_LEFTSHIFT, "Shift_L");
    add(KEY_RIGHTSHIFT, "Shift_R");
    add(KEY_LEFTCTRL, "Control_L");
    add(KEY_RIGHTCTRL, "Control_R");
    add(KEY_LEFTALT, "Alt_L");
    add(KEY_RIGHTALT, "Alt_R");

    for (int c = KEY_F1; c <= KEY_F24; ++c) {
        map.insert(c, QStringLiteral("F%1").arg(c - KEY_F1 + 1));
    }
    for (int c = KEY_A; c <= KEY_Z; ++c) {
        const char letter = static_cast<char>('a' + (c - KEY_A));
        map.insert(c, QString(QChar(letter)));
    }
    for (int c = KEY_0; c <= KEY_9; ++c) {
        const char digit = static_cast<char>('0' + (c - KEY_0));
        map.insert(c, QString(QChar(digit)));
    }

    return map;
}

} // namespace

std::optional<int> evdevCodeToX11MouseButton(int evdevCode) {
    if (!isEvdevButtonCode(evdevCode)) {
        return std::nullopt;
    }
    static const QHash<int, int> table = buildEvdevToXButton();
    const auto it = table.constFind(evdevCode);
    if (it != table.constEnd()) {
        return *it;
    }
    return std::nullopt;
}

std::optional<QString> evdevCodeToXKeysymName(int evdevCode) {
    if (!isEvdevKeyCode(evdevCode)) {
        return std::nullopt;
    }
    static const QHash<int, QString> table = buildEvdevToKeysym();
    const auto it = table.constFind(evdevCode);
    if (it != table.constEnd()) {
        return *it;
    }
    return std::nullopt;
}
