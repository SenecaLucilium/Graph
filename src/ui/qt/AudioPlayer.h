#pragma once

#include <QObject>

class QAudioSink;
class QIODevice;

namespace src::ui::qt
{

class AudioPlayer : public QObject
{
public:
    explicit AudioPlayer(QObject* parent = nullptr);
    ~AudioPlayer() override;
    bool isAvailable() const;
    void setMuted(bool muted);
    void setVolume(double volume);
    void setPitch(double frequency);
    void start();
    void stop();

private:
    QAudioSink* sink_ = nullptr;
    QIODevice* device_ = nullptr;
    bool available_ = false;
    bool muted_ = false;
    double volume_ = 0.6;
    double pitch_ = 220.0;
};

}
