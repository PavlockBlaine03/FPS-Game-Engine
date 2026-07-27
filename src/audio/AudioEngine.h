#pragma once

#include <miniaudio.h>
#include <string>

class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    AudioEngine(AudioEngine&&) = delete;
    AudioEngine& operator=(AudioEngine&&) = delete;

    void play(const std::string& filePath);
    void setMasterVolume(float volume);

private:
    ma_engine m_engine{};
};