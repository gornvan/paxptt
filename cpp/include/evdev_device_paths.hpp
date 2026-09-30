#pragma once

#include <QStringList>

/// Stable by-id nodes plus best-effort discovery from /proc/bus/input/devices.
QStringList discoverEvdevInputDevicePaths();
