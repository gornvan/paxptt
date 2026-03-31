#pragma once

#include <QStringList>

class PulseAudioController {
public:
    QStringList recordingSources() const;
    void muteAllRecordingSources() const;
    void unmuteAllRecordingSources() const;
};
