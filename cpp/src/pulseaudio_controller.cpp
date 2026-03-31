#include "pulseaudio_controller.hpp"

#include <QDebug>
#include <QProcess>

PulseAudioController::PulseAudioController(bool cacheInputs)
    : m_cacheInputs(cacheInputs) {
}

QStringList PulseAudioController::recordingSources() const {
    if (!m_cacheInputs) {
        return queryRecordingSources();
    }

    if (!m_sourcesCached) {
        m_cachedSources = queryRecordingSources();
        m_sourcesCached = true;
    }

    return m_cachedSources;
}

QStringList PulseAudioController::queryRecordingSources() const {
    QProcess process;
    process.start("pactl", {"list", "sources", "short"});
    if (!process.waitForFinished(3000)) {
        qWarning() << "Timed out while reading PulseAudio sources";
        return {};
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qWarning() << "Failed to list PulseAudio sources:" << process.readAllStandardError();
        return {};
    }

    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    QStringList sources;
    for (const QString &line : lines) {
        const QStringList columns = line.split('\t');
        if (columns.size() < 2) {
            continue;
        }
        const QString source = columns.at(1).trimmed();
        if (source.contains(".monitor")) {
            continue;
        }
        sources << source;
    }
    return sources;
}

void PulseAudioController::muteAllRecordingSources() const {
    for (const QString &source : recordingSources()) {
        QProcess::execute("pactl", {"set-source-mute", source, "1"});
    }
}

void PulseAudioController::unmuteAllRecordingSources() const {
    for (const QString &source : recordingSources()) {
        QProcess::execute("pactl", {"set-source-mute", source, "0"});
    }
}
