#pragma once

#include <QStringList>

class PulseAudioController {
public:
    explicit PulseAudioController(bool cacheInputs);

    QStringList recordingSources() const;
    void muteAllRecordingSources() const;
    void unmuteAllRecordingSources() const;

private:
    QStringList queryRecordingSources() const;

    bool m_cacheInputs = true;
    mutable bool m_sourcesCached = false;
    mutable QStringList m_cachedSources;
};
