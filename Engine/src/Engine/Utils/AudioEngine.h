#pragma once
#include <string>

// Forward declare ma_engine so we don't have to include miniaudio.h in the header
struct ma_engine;

namespace Engine {

    class AudioEngine
    {
    public:
        static void Init();
        static void Shutdown();

        // New granular control methods
        static void* LoadSound(const std::string& filepath, bool loop);
        static void StartSound(void* soundHandle, float volume, float pitch);
        static void UpdateSound(void* soundHandle, float volume, float pitch, bool loop);
        static void StopSound(void* soundHandle);
        static void UnloadSound(void* soundHandle);
        static bool IsSoundPlaying(void* soundHandle);

        inline static ma_engine* GetEngine() { return s_Engine; }

    private:
        static ma_engine* s_Engine;
    };
}