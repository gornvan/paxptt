#pragma once

#include <QString>
#include <optional>

/// Parse a BIND_PTT token (BTN_*, KEY_*, or decimal) to a Linux EV_KEY code.
std::optional<int> evdevCodeFromToken(const QString &token);

/// True for mouse button EV_KEY codes (BTN_* range).
bool isEvdevButtonCode(int code);

/// True for keyboard EV_KEY codes (KEY_* range, not buttons).
bool isEvdevKeyCode(int code);
