#define MINIAUDIO_IMPLEMENTATION
#include "audio/AudioEngine.h"

#include <iostream>
#include <stdexcept>
#include <string>

AudioEngine::AudioEngine()
{
    const ma_result result = ma_engine_init(nullptr, &m_engine);

    if (result != MA_SUCCESS)
    {
        throw std::runtime_error(
            std::string("Failed to initialize audio engine: ") +
            ma_result_description(result));
    }
}

AudioEngine::~AudioEngine()
{
    ma_engine_uninit(&m_engine);
}

void AudioEngine::play(const std::string& filePath)
{
    const ma_result result =
        ma_engine_play_sound(&m_engine, filePath.c_str(), nullptr);

    if (result != MA_SUCCESS)
    {
        // A failed sound should not terminate the entire game.
        std::cerr
            << "Failed to play sound '" << filePath << "': "
            << ma_result_description(result)
            << '\n';
    }
}

void AudioEngine::setMasterVolume(const float volume)
{
    // Prevent negative volume. Values above 1.0 intentionally allow
    // amplification, although that can cause clipping.
    const float safeVolume = volume < 0.0F ? 0.0F : volume;
    ma_engine_set_volume(&m_engine, safeVolume);
}