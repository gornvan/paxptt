#include "sound_controller.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QtEndian>
#include <cmath>
#include <cstring>

#include <pulse/error.h>
#include <pulse/simple.h>

namespace {

void writeU16LE(QFile &file, quint16 value) {
    const quint16 le = qToLittleEndian(value);
    file.write(reinterpret_cast<const char *>(&le), sizeof(le));
}

void writeU32LE(QFile &file, quint32 value) {
    const quint32 le = qToLittleEndian(value);
    file.write(reinterpret_cast<const char *>(&le), sizeof(le));
}

quint16 readU16LE(const QByteArray &data, int offset) {
    quint16 value = 0;
    std::memcpy(&value, data.constData() + offset, sizeof(value));
    return qFromLittleEndian(value);
}

quint32 readU32LE(const QByteArray &data, int offset) {
    quint32 value = 0;
    std::memcpy(&value, data.constData() + offset, sizeof(value));
    return qFromLittleEndian(value);
}

bool fourCCEquals(const QByteArray &data, int offset, const char *tag) {
    return data.size() >= offset + 4 && std::memcmp(data.constData() + offset, tag, 4) == 0;
}

} // namespace

SoundController::SoundController() {
    soundDir_ = QDir::homePath() + "/.local/paxp2t/sounds";
    mutePath_ = soundDir_ + "/mute.wav";
    unmutePath_ = soundDir_ + "/unmute.wav";
}

void SoundController::ensureAndLoadSounds() {
    ensureSounds();
    loadSounds();
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

void SoundController::loadSounds() {
    muteSound_ = loadWavFile(mutePath_);
    if (!muteSound_.valid) {
        qWarning() << "Failed to load mute sound from" << mutePath_ << "- using generated beep";
        muteSound_ = makeBeepBuffer(50.0, 0.06, 0.04);
    }

    unmuteSound_ = loadWavFile(unmutePath_);
    if (!unmuteSound_.valid) {
        qWarning() << "Failed to load unmute sound from" << unmutePath_ << "- using generated beep";
        unmuteSound_ = makeBeepBuffer(100.0, 0.06, 0.04);
    }
}

void SoundController::playMute() const {
    playBuffer(muteSound_);
}

void SoundController::playUnmute() const {
    playBuffer(unmuteSound_);
}

QByteArray SoundController::generateBeepPcm(double frequencyHz, double durationSeconds, double volume,
                                             quint32 sampleRate) {
    constexpr double pi = 3.14159265358979323846;
    const quint32 sampleCount = static_cast<quint32>(sampleRate * durationSeconds);
    QByteArray pcm;
    pcm.resize(static_cast<int>(sampleCount * sizeof(qint16)));

    for (quint32 i = 0; i < sampleCount; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(sampleRate);
        const double sample = volume * std::sin(2.0 * pi * frequencyHz * t);
        const qint16 pcmSample = static_cast<qint16>(sample * 32767.0);
        const qint16 le = qToLittleEndian(pcmSample);
        std::memcpy(pcm.data() + static_cast<int>(i * sizeof(qint16)), &le, sizeof(le));
    }
    return pcm;
}

void SoundController::generateBeepWav(const QString &path, double frequencyHz, double durationSeconds,
                                      double volume) {
    constexpr quint32 sampleRate = 44100;
    const QByteArray pcm = generateBeepPcm(frequencyHz, durationSeconds, volume, sampleRate);
    const quint16 channels = 1;
    const quint16 bitsPerSample = 16;
    const quint16 blockAlign = channels * (bitsPerSample / 8);
    const quint32 byteRate = sampleRate * blockAlign;
    const quint32 dataSize = static_cast<quint32>(pcm.size());
    const quint32 riffSize = 36 + dataSize;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    file.write("RIFF", 4);
    writeU32LE(file, riffSize);
    file.write("WAVE", 4);

    file.write("fmt ", 4);
    writeU32LE(file, 16); // PCM chunk size
    writeU16LE(file, 1);  // PCM format
    writeU16LE(file, channels);
    writeU32LE(file, sampleRate);
    writeU32LE(file, byteRate);
    writeU16LE(file, blockAlign);
    writeU16LE(file, bitsPerSample);

    file.write("data", 4);
    writeU32LE(file, dataSize);
    file.write(pcm);
}

SoundController::SoundBuffer SoundController::makeBeepBuffer(double frequencyHz, double durationSeconds,
                                                             double volume) {
    SoundBuffer buffer;
    buffer.spec.format = PA_SAMPLE_S16LE;
    buffer.spec.rate = 44100;
    buffer.spec.channels = 1;
    buffer.pcm = generateBeepPcm(frequencyHz, durationSeconds, volume, buffer.spec.rate);
    buffer.valid = !buffer.pcm.isEmpty();
    return buffer;
}

SoundController::SoundBuffer SoundController::loadWavFile(const QString &path) {
    SoundBuffer buffer;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open sound file:" << path;
        return buffer;
    }

    const QByteArray data = file.readAll();
    if (data.size() < 44 || !fourCCEquals(data, 0, "RIFF") || !fourCCEquals(data, 8, "WAVE")) {
        qWarning() << "Sound file is not a valid WAV:" << path;
        return buffer;
    }

    int offset = 12;
    quint16 audioFormat = 0;
    quint16 channels = 0;
    quint32 sampleRate = 0;
    quint16 bitsPerSample = 0;
    int dataOffset = -1;
    quint32 dataSize = 0;

    while (offset + 8 <= data.size()) {
        const char *chunkId = data.constData() + offset;
        const quint32 chunkSize = readU32LE(data, offset + 4);
        const int chunkData = offset + 8;

        if (std::memcmp(chunkId, "fmt ", 4) == 0) {
            if (chunkSize < 16 || chunkData + 16 > data.size()) {
                qWarning() << "Invalid fmt chunk in" << path;
                return buffer;
            }
            audioFormat = readU16LE(data, chunkData);
            channels = readU16LE(data, chunkData + 2);
            sampleRate = readU32LE(data, chunkData + 4);
            bitsPerSample = readU16LE(data, chunkData + 14);
        } else if (std::memcmp(chunkId, "data", 4) == 0) {
            dataOffset = chunkData;
            dataSize = chunkSize;
            break;
        }

        // Chunk sizes are word-aligned
        offset = chunkData + static_cast<int>((chunkSize + 1) & ~1u);
    }

    if (dataOffset < 0 || audioFormat == 0) {
        qWarning() << "WAV missing fmt/data chunks:" << path;
        return buffer;
    }

    if (audioFormat != 1 || bitsPerSample != 16 || (channels != 1 && channels != 2)) {
        qWarning() << "Unsupported WAV format in" << path
                    << "(need PCM16 mono/stereo; got format=" << audioFormat
                    << "bits=" << bitsPerSample << "channels=" << channels << ")";
        return buffer;
    }

    if (sampleRate == 0 || dataOffset + static_cast<int>(dataSize) > data.size()) {
        qWarning() << "Invalid WAV data size in" << path;
        return buffer;
    }

    buffer.spec.format = PA_SAMPLE_S16LE;
    buffer.spec.rate = sampleRate;
    buffer.spec.channels = static_cast<uint8_t>(channels);
    buffer.pcm = data.mid(dataOffset, static_cast<int>(dataSize));
    buffer.valid = !buffer.pcm.isEmpty();
    return buffer;
}

void SoundController::playBuffer(const SoundBuffer &buffer) const {
    if (!buffer.valid || buffer.pcm.isEmpty()) {
        return;
    }

    int error = 0;
    pa_simple *stream = pa_simple_new(nullptr, "paxp2t", PA_STREAM_PLAYBACK, nullptr, "indicator",
                                      &buffer.spec, nullptr, nullptr, &error);
    if (!stream) {
        if (!playbackWarned_) {
            qWarning() << "PulseAudio playback unavailable:" << pa_strerror(error);
            playbackWarned_ = true;
        }
        return;
    }

    if (pa_simple_write(stream, buffer.pcm.constData(), static_cast<size_t>(buffer.pcm.size()), &error) < 0) {
        if (!playbackWarned_) {
            qWarning() << "PulseAudio write failed:" << pa_strerror(error);
            playbackWarned_ = true;
        }
        pa_simple_free(stream);
        return;
    }

    if (pa_simple_drain(stream, &error) < 0) {
        if (!playbackWarned_) {
            qWarning() << "PulseAudio drain failed:" << pa_strerror(error);
            playbackWarned_ = true;
        }
    }

    pa_simple_free(stream);
}
