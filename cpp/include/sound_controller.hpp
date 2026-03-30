#pragma once

#include <QString>

class SoundController {
public:
    SoundController();

    void ensureSounds() const;
    void playMute() const;
    void playUnmute() const;

private:
    QString soundDir_;
    QString mutePath_;
    QString unmutePath_;

    static void generateBeepWav(const QString &path, double frequencyHz, double durationSeconds, double volume);
    static void playFile(const QString &path);
};
