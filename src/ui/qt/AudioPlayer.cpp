#include "AudioPlayer.h"

#ifdef GRAPH_HAS_QT_MULTIMEDIA
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QMutex>
#include <QMutexLocker>

#include <algorithm>
#include <cmath>
#include <cstdint>
#endif

namespace src::ui::qt
{
#ifdef GRAPH_HAS_QT_MULTIMEDIA
namespace
{

constexpr int sampleRate = 44100;
constexpr double twoPi = 6.28318530717958647692;

QAudioFormat createAudioFormat()
{
    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    return format;
}

class ToneDevice final : public QIODevice
{
public:
    ToneDevice() { open(QIODevice::ReadOnly); }

    void setTone(double frequency, double amplitude)
    {
        QMutexLocker locker(&mutex_);
        frequency_ = std::clamp(frequency, 40.0, 2400.0);
        amplitude_ = std::clamp(amplitude, 0.0, 1.0);
    }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        if (maxSize <= 0) return 0;

        QMutexLocker locker(&mutex_);
        const qint64 sampleCount = maxSize / static_cast<qint64>(sizeof(std::int16_t));
        auto* output = reinterpret_cast<std::int16_t*>(data);

        for (qint64 index = 0; index < sampleCount; ++index)
        {
            const double sample = std::sin(phase_) * amplitude_;
            output[index] = static_cast<std::int16_t>(std::clamp(sample, -1.0, 1.0) * 30000.0);
            phase_ += twoPi * frequency_ / static_cast<double>(sampleRate);
            if (phase_ >= twoPi) phase_ -= twoPi;
        }

        return sampleCount * static_cast<qint64>(sizeof(std::int16_t));
    }

    qint64 writeData(const char*, qint64) override { return -1; }
    qint64 bytesAvailable() const override { return 4096 + QIODevice::bytesAvailable(); }

private:
    mutable QMutex mutex_;
    double frequency_ = 220.0;
    double amplitude_ = 0.0;
    double phase_ = 0.0;
};

}

AudioPlayer::AudioPlayer(QObject* parent) : QObject(parent)
{
    auto* device = new ToneDevice();
    auto* sink = new QAudioSink(createAudioFormat(), this);
    sink->setVolume(static_cast<float>(volume_));
    device_ = device;
    sink_ = sink;
    available_ = true;
    static_cast<ToneDevice*>(device_)->setTone(220.0, 0.0);
}

AudioPlayer::~AudioPlayer()
{
    stop();
    delete device_;
}

bool AudioPlayer::isAvailable() const
{
    return available_;
}

void AudioPlayer::setMuted(bool muted)
{
    muted_ = muted;
    static_cast<ToneDevice*>(device_)->setTone(pitch_, muted_ ? 0.0 : volume_);
}

void AudioPlayer::setVolume(double volume)
{
    volume_ = std::clamp(volume, 0.0, 1.0);
    sink_->setVolume(static_cast<float>(volume_));
    static_cast<ToneDevice*>(device_)->setTone(pitch_, muted_ ? 0.0 : volume_);
}

void AudioPlayer::setPitch(double frequency)
{
    pitch_ = frequency;
    static_cast<ToneDevice*>(device_)->setTone(pitch_, muted_ ? 0.0 : volume_);
}

void AudioPlayer::start()
{
    if (!available_) return;
    if (sink_ != nullptr && sink_->state() == QAudio::ActiveState) return;
    if (sink_ != nullptr)
    {
        sink_->stop();
        delete sink_;
    }

    sink_ = new QAudioSink(createAudioFormat(), this);
    sink_->setVolume(static_cast<float>(volume_));
    sink_->start(device_);
}

void AudioPlayer::stop()
{
    if (sink_ != nullptr) sink_->stop();
    if (device_ != nullptr) static_cast<ToneDevice*>(device_)->setTone(pitch_, 0.0);
}

#else

AudioPlayer::AudioPlayer(QObject* parent) : QObject(parent) {}

AudioPlayer::~AudioPlayer() = default;

bool AudioPlayer::isAvailable() const
{
    return false;
}

void AudioPlayer::setMuted(bool) {}
void AudioPlayer::setVolume(double) {}
void AudioPlayer::setPitch(double) {}
void AudioPlayer::start() {}
void AudioPlayer::stop() {}

#endif

}
