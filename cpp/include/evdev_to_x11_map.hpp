#pragma once

#include <QString>
#include <optional>

/// Map an evdev EV_KEY code to an X11 pointer button number (XRecord).
std::optional<int> evdevCodeToX11MouseButton(int evdevCode);

/// Map an evdev EV_KEY code to an X11 keysym name for XStringToKeysym.
std::optional<QString> evdevCodeToXKeysymName(int evdevCode);
