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

void AudioEngine::play(AudioType audio)
{
    std::string filePath;
    ma_result result;
    switch (audio)
    {
    case DOOR_CLOSING:
        filePath = "assets/audio/door/door-closing.wav";
        result = ma_engine_play_sound(&m_engine, filePath.c_str(), nullptr);

        if (result != MA_SUCCESS)
        {
            // A failed sound should not terminate the entire game.
            std::cerr
                << "Failed to play sound '" << filePath.c_str() << "': "
                << ma_result_description(result)
                << '\n';
        }
        break;
    case DOOR_OPENING:
        filePath = "assets/audio/door/door-opening.wav";
        result = ma_engine_play_sound(&m_engine, filePath.c_str(), nullptr);

        if (result != MA_SUCCESS)
        {
            // A failed sound should not terminate the entire game.
            std::cerr
                << "Failed to play sound '" << filePath.c_str() << "': "
                << ma_result_description(result)
                << '\n';
        }
        break;
    case PISTOL_SHOT:
        filePath = "assets/audio/weapons/pistol-shot.wav";
        result = ma_engine_play_sound(&m_engine, filePath.c_str(), nullptr);

        if (result != MA_SUCCESS)
        {
            // A failed sound should not terminate the entire game.
            std::cerr
                << "Failed to play sound '" << filePath << "': "
                << ma_result_description(result)
                << '\n';
        }
        break;
    default:
        std::cerr << "INVALID SOUND TYPE" << "\n";
    }

}

void AudioEngine::setMasterVolume(const float volume)
{
    // Prevent negative volume. Values above 1.0 intentionally allow
    // amplification, although that can cause clipping.
    const float safeVolume = volume < 0.0F ? 0.0F : volume;
    ma_engine_set_volume(&m_engine, safeVolume);
}