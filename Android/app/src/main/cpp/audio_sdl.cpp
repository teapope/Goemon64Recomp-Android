
#include <SDL.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "GOEMON_AUD", __VA_ARGS__)

static SDL_AudioDeviceID gDev = 0;
static SDL_AudioSpec gObt = {};

static void AudioCallback(void* userdata, Uint8* stream, int len) {
    // TODO: call into Goemon mixer to fill 'stream' with 'len' bytes
    SDL_memset(stream, 0, len);
}

bool Audio_Init(int freq = 48000, SDL_AudioFormat fmt = AUDIO_S16SYS, int chans = 2, int samples = 1024) {
    SDL_AudioSpec want{};
    want.freq = freq; want.format = fmt; want.channels = (Uint8)chans; want.samples = samples;
    want.callback = AudioCallback; want.userdata = nullptr;
    gDev = SDL_OpenAudioDevice(nullptr, 0, &want, &gObt, 0);
    if (!gDev) return false;
    SDL_PauseAudioDevice(gDev, 0);
    LOGI("Audio started: %d Hz, %d ch", gObt.freq, gObt.channels);
    return true;
}

void Audio_Shutdown() {
    if (gDev) { SDL_CloseAudioDevice(gDev); gDev = 0; }
}
