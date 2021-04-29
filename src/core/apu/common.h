#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>


#define SQUARE1_CHANNEL gb->apu.square1
#define SQUARE2_CHANNEL gb->apu.square2
#define WAVE_CHANNEL gb->apu.wave
#define NOISE_CHANNEL gb->apu.noise
#define CONTROL_CHANNEL gb->apu.control

#ifndef GB_AUDIO_FREQUENCY
#define GB_AUDIO_FREQUENCY 22050
#endif
// 48000

#ifdef GB_SDL_AUDIO_CALLBACK_STREAM
#define SAMPLE_RATE 4
#else
#define SAMPLE_RATE (4213440 / GB_AUDIO_FREQUENCY)
#endif

// clocked at 512hz
#define FRAME_SEQUENCER_CLOCK 512

// 4 * 1024^2 / 512
#define FRAME_SEQUENCER_STEP_RATE 8192


enum EnvelopeMode {
    SUB = 0,
    ADD = 1
};


// defined in core/apu/apu.c
extern const bool SQUARE_DUTY_CYCLES[4][8];

// defined in core/apu/apu.c
extern const uint8_t PERIOD_TABLE[8];


#ifdef __cplusplus
}
#endif
