// AudioEngine.cpp
#include "egpch.h"
#include "miniaudio/miniaudio.h"
#include "AudioEngine.h"

namespace Engine {
    ma_engine* AudioEngine::s_Engine = nullptr;

    void AudioEngine::Init()
    {
        s_Engine = new ma_engine();
        ma_result result = ma_engine_init(NULL, s_Engine);
        if (result != MA_SUCCESS) {
            ENGINE_LOG_ERROR("Failed to initialize Miniaudio!");
            return;
        }
        ENGINE_LOG_INFO("Audio Engine Initialized Successfully.");
    }

    void AudioEngine::Shutdown()
    {
        if (s_Engine) {
            ma_engine_uninit(s_Engine);
            delete s_Engine;
            s_Engine = nullptr;
        }
    }

    void* AudioEngine::LoadSound(const std::string& filepath, bool loop)
    {
        if (!s_Engine) return nullptr;

        // Allocate memory for the sound object
        ma_sound* sound = new ma_sound();

        // Initialize the sound from the file
        ma_result result = ma_sound_init_from_file(s_Engine, filepath.c_str(), 0, NULL, NULL, sound);
        if (result != MA_SUCCESS)
        {
            ENGINE_LOG_ERROR("Failed to load sound: {}", filepath);
            delete sound;
            return nullptr;
        }

        // Apply the loop property right at initialization
        ma_sound_set_looping(sound, loop ? MA_TRUE : MA_FALSE);

        return sound; // Return as void* to the component
    }

    void AudioEngine::StartSound(void* soundHandle, float volume, float pitch)
    {
        if (!soundHandle) return;

        ma_sound* sound = static_cast<ma_sound*>(soundHandle);

        // Apply your component's properties dynamically before playing!
        ma_sound_set_volume(sound, volume);
        ma_sound_set_pitch(sound, pitch);

        // If the sound was already playing or paused, this restarts/resumes it
        // Rewind to the beginning before playing
        ma_sound_seek_to_pcm_frame(sound, 0);
        ma_sound_start(sound);
    }

    void AudioEngine::UpdateSound(void* soundHandle, float volume, float pitch, bool loop)
    {
        if (!soundHandle) return;

        ma_sound* sound = static_cast<ma_sound*>(soundHandle);

        ma_sound_set_volume(sound, volume);
        ma_sound_set_pitch(sound, pitch);
        ma_sound_set_looping(sound, loop ? MA_TRUE : MA_FALSE);
    }

    void AudioEngine::StopSound(void* soundHandle)
    {
        if (!soundHandle) return;

        ma_sound* sound = static_cast<ma_sound*>(soundHandle);

        // This tells miniaudio to pause the playback exactly where it is.
        ma_sound_stop(sound);
    }

    void AudioEngine::UnloadSound(void* soundHandle)
    {
        if (!soundHandle) return;
        ma_sound* sound = static_cast<ma_sound*>(soundHandle);
        ma_sound_uninit(sound);
        delete sound;
    }

    bool AudioEngine::IsSoundPlaying(void* soundHandle)
    {
        if (!soundHandle) return false;
        ma_sound* sound = static_cast<ma_sound*>(soundHandle);

        // miniaudio's built-in thread-safe check
        return ma_sound_is_playing(sound) == MA_TRUE;
    }
}