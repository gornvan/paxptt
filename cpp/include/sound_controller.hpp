#pragma once

#include <QByteArray>
#include <QString>

#include <pulse/sample.h>

class SoundController {
public:
    SoundController();

    void ensureAndLoadSounds();
    void playMute() const;
    void playUnmute() const;

private:
    struct SoundBuffer {
        QByteArray pcm;
        pa_sample_spec spec{};
        bool valid = false;
    };

    QString soundDir_;
    QString mutePath_;
    QString unmutePath_;
    SoundBuffer muteSound_;
    SoundBuffer unmuteSound_;
    mutable bool playbackWarned_ = false;

    void ensureSounds() const;
    void loadSounds();
    static QByteArray generateBeepPcm(double frequencyHz, double durationSeconds, double volume,
                                      quint32 sampleRate = 44100);
    static void generateBeepWav(const QString &path, double frequencyHz, double durationSeconds, double volume);
    static SoundBuffer loadWavFile(const QString &path);
    static SoundBuffer makeBeepBuffer(double frequencyHz, double durationSeconds, double volume);
    void playBuffer(const SoundBuffer &buffer) const;
};
