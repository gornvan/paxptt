#include "sound_controller.hpp"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QtEndian>
#include <cmath>
#include <cstdint>

namespace {

void writeU16LE(QFile &file, quint16 value) {
    const quint16 le = qToLittleEndian(value);
    file.write(reinterpret_cast<const char *>(&le), sizeof(le));
}

void writeU32LE(QFile &file, quint32 value) {
    const quint32 le = qToLittleEndian(value);
    file.write(reinterpret_cast<const char *>(&le), sizeof(le));
}

} // namespace

SoundController::SoundController() {
    soundDir_ = QDir::homePath() + "/.local/paxp2t/sounds";
    mutePath_ = soundDir_ + "/mute.wav";
    unmutePath_ = soundDir_ + "/unmute.wav";
}

void SoundController::ensureSounds() const {
    QDir dir(soundDir_);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if (!QFile::exists(unmutePath_)) {
        generateBeepWav(unmutePath_, 100.0, 0.06, 0.04);
    }
    if (!QFile::exists(mutePath_)) {
        generateBeepWav(mutePath_, 50.0, 0.06, 0.04);
    }
}

void SoundController::playMute() const {
    playFile(mutePath_);
}

void SoundController::playUnmute() const {
    playFile(unmutePath_);
}

void SoundController::generateBeepWav(const QString &path, double frequencyHz, double durationSeconds, double volume) {
    constexpr quint32 sampleRate = 44100;
    constexpr double pi = 3.14159265358979323846;
    const quint32 sampleCount = static_cast<quint32>(sampleRate * durationSeconds);
    const quint16 channels = 1;
    const quint16 bitsPerSample = 16;
    const quint16 blockAlign = channels * (bitsPerSample / 8);
    const quint32 byteRate = sampleRate * blockAlign;
    const quint32 dataSize = sampleCount * blockAlign;
    const quint32 riffSize = 36 + dataSize;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    file.write("RIFF", 4);
    writeU32LE(file, riffSize);
    file.write("WAVE", 4);

    file.write("fmt ", 4);
    writeU32LE(file, 16);            // PCM chunk size
    writeU16LE(file, 1);             // PCM format
    writeU16LE(file, channels);
    writeU32LE(file, sampleRate);
    writeU32LE(file, byteRate);
    writeU16LE(file, blockAlign);
    writeU16LE(file, bitsPerSample);

    file.write("data", 4);
    writeU32LE(file, dataSize);

    for (quint32 i = 0; i < sampleCount; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(sampleRate);
        const double sample = volume * std::sin(2.0 * pi * frequencyHz * t);
        const qint16 pcm = static_cast<qint16>(sample * 32767.0);
        const qint16 le = qToLittleEndian(pcm);
        file.write(reinterpret_cast<const char *>(&le), sizeof(le));
    }
}

void SoundController::playFile(const QString &path) {
    QProcess::startDetached("paplay", {path});
}
