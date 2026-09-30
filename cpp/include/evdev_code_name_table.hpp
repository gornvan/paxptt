#pragma once

#include <QHash>
#include <QString>

/// BIND_PTT token (KEY_* / BTN_*) → Linux EV_KEY code. Generated from vendored input-event-codes.h.
const QHash<QString, int> &evdevNameToCodeTable();
