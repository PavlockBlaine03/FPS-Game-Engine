#pragma once

#include <miniaudio.h>
#include <string>

class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    enum AudioType
    {
        DOOR_CLOSING,
        DOOR_OPENING,
        PISTOL_SHOT
    };

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    AudioEngine(AudioEngine&&) = delete;
    AudioEngine& operator=(AudioEngine&&) = delete;

    void play(AudioType audio);
    void setMasterVolume(float volume);

private:
    ma_engine m_engine{};
};